//-------------------------------------------------------------
//! @file   AnimationSystem.cpp
//! @brief  AnimationSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/AnimationSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkeletonOutputComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/NodeWorldPoseComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/NodeWorldMatrixComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Model/ModelAsset.hpp>
#include <Tsukino/Engine/Physics/SpringBone/SpringBonePhysics.hpp>
#include <Tsukino/GraphicsCommon/Model/ModelData.hpp>
#include <Tsukino/Core/Log.hpp>

#include <entt/entt.hpp>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace Tsukino::BuiltIn::ECS {

    //-------------------------------------------------------------
    //! @brief ノード→チャンネルの対応表を取得する（無ければ構築してキャッシュ）
    //-------------------------------------------------------------
    const std::vector<std::int32_t>& AnimationSystem::ResolveChannelTable(const Tsukino::GraphicsCommon::AnimationData& animation,
                                                                          const Tsukino::GraphicsCommon::ModelData&     model) {
        const ChannelTableKey key{&animation, &model};

        auto it = m_channelTables.find(key);
        if(it != m_channelTables.end())
            return it->second;

        //---------------------------------------------------------
        // まずチャンネル名→チャンネルindexの表を作り、それをノード順に引き直す。
        // 素直に二重ループで作るとここもO(ノード数×チャンネル数)になるため、
        // 一度だけとはいえハッシュで引く
        //
        // 同名チャンネルが複数あった場合は先に現れた方を採る。
        // 以前の線形探索がbreakで最初の一致を採用していたのと挙動を揃えるため
        //---------------------------------------------------------
        std::unordered_map<std::string, std::int32_t> channelIndexByName;
        channelIndexByName.reserve(animation.channels.size());

        for(std::size_t channelIndex = 0; channelIndex < animation.channels.size(); ++channelIndex) {
            channelIndexByName.emplace(animation.channels[channelIndex].nodeName, static_cast<std::int32_t>(channelIndex));
        }

        std::vector<std::int32_t> table(model.nodes.size(), -1);
        for(std::size_t nodeIndex = 0; nodeIndex < model.nodes.size(); ++nodeIndex) {
            const auto found = channelIndexByName.find(model.nodes[nodeIndex].name);
            if(found != channelIndexByName.end())
                table[nodeIndex] = found->second;
        }

        return m_channelTables.emplace(key, std::move(table)).first->second;
    }

    //-------------------------------------------------------------
    //! @brief ベクトルの線形補間（位置・スケール用）
    //! @param keys キーフレームのリスト
    //! @param time 現在のアニメーション時間（Ticks）
    //! @return 補間されたhlslpp::float3
    //-------------------------------------------------------------
    static hlslpp::float3 LerpVector(const std::vector<Tsukino::GraphicsCommon::VectorKey>& keys, float time) {
        if(keys.empty())
            return hlslpp::float3(0, 0, 0);
        if(keys.size() == 1 || time <= keys.front().time)
            return hlslpp::float3(keys.front().value.x, keys.front().value.y, keys.front().value.z);
        if(time >= keys.back().time)
            return hlslpp::float3(keys.back().value.x, keys.back().value.y, keys.back().value.z);

        for(size_t i = 0; i < keys.size() - 1; ++i) {
            if(time >= keys[i].time && time < keys[i + 1].time) {
                float t = (time - keys[i].time) / (keys[i + 1].time - keys[i].time);
                return hlslpp::lerp(hlslpp::float3(keys[i].value.x, keys[i].value.y, keys[i].value.z),
                                    hlslpp::float3(keys[i + 1].value.x, keys[i + 1].value.y, keys[i + 1].value.z),
                                    hlslpp::float3(t, t, t));
            }
        }
        return hlslpp::float3(keys.back().value.x, keys.back().value.y, keys.back().value.z);
    }

    //-------------------------------------------------------------
    //! @brief クォータニオンの球面線形補間（回転用）
    //! @param keys キーフレームのリスト
    //! @param time 現在のアニメーション時間（Ticks）
    //! @return 補間されたhlslpp::quaternion
    //-------------------------------------------------------------
    static hlslpp::quaternion SlerpQuaternion(const std::vector<Tsukino::GraphicsCommon::QuaternionKey>& keys, float time) {
        if(keys.empty())
            return hlslpp::quaternion(0, 0, 0, 1);
        if(keys.size() == 1 || time <= keys.front().time)
            return hlslpp::quaternion(keys.front().value.x, keys.front().value.y, keys.front().value.z, keys.front().value.w);
        if(time >= keys.back().time)
            return hlslpp::quaternion(keys.back().value.x, keys.back().value.y, keys.back().value.z, keys.back().value.w);

        for(size_t i = 0; i < keys.size() - 1; ++i) {
            if(time >= keys[i].time && time < keys[i + 1].time) {
                float              t  = (time - keys[i].time) / (keys[i + 1].time - keys[i].time);
                hlslpp::quaternion q1 = hlslpp::quaternion(keys[i].value.x, keys[i].value.y, keys[i].value.z, keys[i].value.w);
                hlslpp::quaternion q2 = hlslpp::quaternion(keys[i + 1].value.x, keys[i + 1].value.y, keys[i + 1].value.z, keys[i + 1].value.w);

                // 内積が負なら q2 を反転して最短経路を保証
                float dot = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
                if(dot < 0.0f) {
                    q2 = hlslpp::quaternion(-q2.x, -q2.y, -q2.z, -q2.w);
                }

                return hlslpp::slerp(q1, q2, t);
            }
        }
        return hlslpp::quaternion(keys.back().value.x, keys.back().value.y, keys.back().value.z, keys.back().value.w);
    }

    //-------------------------------------------------------------
    //! @struct ClipRangeTicks
    //! @brief  クリップ内の再生レンジ（秒指定）をtickへ変換した結果
    //-------------------------------------------------------------
    struct ClipRangeTicks {
        float start;
        float end;
    };

    //-------------------------------------------------------------
    //! @brief  再生レンジ（秒）をクリップ内のtick範囲へ変換する
    //! @param  anim     [in] 評価対象のアニメーション
    //! @param  startSec [in] レンジ開始（秒）
    //! @param  endSec   [in] レンジ終了（秒）。0以下ならクリップ末尾
    //! @return レンジのtick範囲
    //-------------------------------------------------------------
    static ClipRangeTicks ComputeRangeTicks(const Tsukino::GraphicsCommon::AnimationData& anim, float startSec, float endSec) {
        const float startTicks = std::clamp(startSec * anim.ticksPerSecond, 0.0f, anim.duration);
        const float endTicks   = (endSec > 0.0f) ? std::clamp(endSec * anim.ticksPerSecond, startTicks, anim.duration)
                                                  : anim.duration;
        return {startTicks, endTicks};
    }

    //-------------------------------------------------------------
    //! @brief  レンジ先頭からの経過秒を、クリップ内の絶対tick時刻へ写像する
    //! @param  anim        [in]  評価対象のアニメーション
    //! @param  elapsedSec  [in]  レンジ先頭からの経過時間（秒）
    //! @param  startSec    [in]  レンジ開始（秒）
    //! @param  endSec      [in]  レンジ終了（秒）。0以下ならクリップ末尾
    //! @param  looping     [in]  レンジ内でループするか
    //! @param  outFinished [out] 非ループ時にレンジ末尾へ到達したか
    //! @return クリップ内の絶対tick時刻
    //-------------------------------------------------------------
    static float EvaluateClipTicks(const Tsukino::GraphicsCommon::AnimationData& anim,
                                   float elapsedSec, float startSec, float endSec,
                                   bool looping, bool& outFinished) {
        const ClipRangeTicks range      = ComputeRangeTicks(anim, startSec, endSec);
        // レンジ長が0だとfmodがNaNを返し、ポーズ全体が壊れる（開始=終了の設定ミスへの保険）
        const float           rangeTicks = std::max(range.end - range.start, 0.0001f);

        const float localTicks = elapsedSec * anim.ticksPerSecond;
        if(!looping && localTicks >= rangeTicks) {
            outFinished = true;
            return range.end;
        }
        outFinished = false;
        return range.start + std::fmod(localTicks, rangeTicks);
    }

    //-------------------------------------------------------------
    //! @brief  In Placeで固定する対象ノード（ルートモーションノード）のindexを決める
    //! @param  model    [in] スケルトンの取得元となるモデルデータ
    //! @param  nameHint [in] 明示指定されたノード名。空なら自動判定する
    //! @return ノードindex。見つからなければUINT32_MAX
    //-------------------------------------------------------------
    static u32 ResolveRootMotionNode(const Tsukino::GraphicsCommon::ModelData& model, const std::string& nameHint) {
        if(!nameHint.empty()) {
            for(u32 i = 0; i < model.nodes.size(); ++i) {
                if(model.nodes[i].name == nameHint) {
                    return i;
                }
            }
            return UINT32_MAX;
        }

        // 自動判定：スキニング対象ボーン（skeleton.bones）のうち、祖先に他のボーンを
        // 持たない最も浅いものを選ぶ。Mixamoの場合Armature/RootNodeはボーンではないため
        // "mixamorig:Hips" が選ばれる
        std::unordered_set<u32> boneNodeIndices;
        for(const auto& bone : model.skeleton.bones) {
            if(bone.nodeIndex != UINT32_MAX) {
                boneNodeIndices.insert(bone.nodeIndex);
            }
        }

        u32 bestIndex = UINT32_MAX;
        u32 bestDepth = UINT32_MAX;
        for(u32 nodeIndex : boneNodeIndices) {
            u32 depth  = 0;
            u32 walker = model.nodes[nodeIndex].parentIndex;
            while(walker != UINT32_MAX && walker < model.nodes.size()) {
                ++depth;
                walker = model.nodes[walker].parentIndex;
            }
            if(depth < bestDepth) {
                bestDepth = depth;
                bestIndex = nodeIndex;
            }
        }
        return bestIndex;
    }

    //-------------------------------------------------------------
    //! @brief アニメーションシステムのメイン更新処理
    //-------------------------------------------------------------
    void AnimationSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->assetManager)
            return;

        auto view = registry.View<AnimationPlayerComponent, SkeletonOutputComponent>();

        view.each([&](entt::entity entity, AnimationPlayerComponent& player, SkeletonOutputComponent& skeletonOut) {
            if(player.is_playing) {
                player.elapsed_time += deltaTime * player.playback_speed;
            }

            AnimationControllerComponent* controller = nullptr;
            if(registry.HasComponent<AnimationControllerComponent>(entity)) {
                controller = &registry.GetComponent<AnimationControllerComponent>(entity);
            }

            if(controller && controller->next.clip.IsValid()) {
                if(controller->next.immediate) {
                    player.current_clip_id       = controller->next.clip;
                    player.animation_index       = controller->next.animation_index;
                    player.elapsed_time          = 0.0f;
                    player.is_looping            = controller->next.is_looping;
                    player.clip_start_time       = controller->next.clip_start_time;
                    player.clip_end_time         = controller->next.clip_end_time;
                    player.in_place              = controller->next.in_place;
                    controller->is_transitioning = false;
                    controller->blend_alpha      = 0.0f;
                } else {
                    // 新クリップを即座に時間0から再生開始し、旧クリップ（現在playerが指していたもの）は
                    // outgoingとしてスナップショットして自分の経過時間を保ったまま並行フェードアウトさせる。
                    // 遷移中に更に別の遷移が来た場合も、その時点でplayerが指しているクリップが新たなoutgoingに
                    // なるだけなので同じロジックで自然に処理される。
                    controller->outgoing.clip            = player.current_clip_id;
                    controller->outgoing.animation_index = player.animation_index;
                    controller->outgoing.elapsed_time    = player.elapsed_time;
                    controller->outgoing.is_looping       = player.is_looping;
                    controller->outgoing.clip_start_time = player.clip_start_time;
                    controller->outgoing.clip_end_time   = player.clip_end_time;
                    controller->outgoing.in_place         = player.in_place;

                    player.current_clip_id = controller->next.clip;
                    player.animation_index = controller->next.animation_index;
                    player.elapsed_time    = 0.0f;
                    player.is_looping      = controller->next.is_looping;
                    player.clip_start_time = controller->next.clip_start_time;
                    player.clip_end_time   = controller->next.clip_end_time;
                    player.in_place        = controller->next.in_place;

                    controller->is_transitioning = true;
                    controller->blend_alpha      = 0.0f;
                }
                controller->next.clip = Tsukino::Asset::AssetHandle{};    // 消費済み（fade_timeはブレンド中に参照するため残す）
            }

            if(controller && controller->is_transitioning) {
                controller->blend_alpha += deltaTime / std::max(controller->next.fade_time, 0.0001f);
                if(player.is_playing) {
                    controller->outgoing.elapsed_time += deltaTime * player.playback_speed;
                }
                if(controller->blend_alpha >= 1.0f) {
                    controller->blend_alpha      = 1.0f;
                    controller->is_transitioning = false;
                }
            }

            if(!player.current_clip_id.IsValid())
                return;

            auto animAsset = ctx->assetManager->Get(player.current_clip_id);
            if(!animAsset || animAsset->GetType() != Tsukino::Asset::AssetType::Model)
                return;

            auto modelAss = std::static_pointer_cast<Tsukino::Asset::ModelAsset>(animAsset);
            if(modelAss->modelData.animations.empty())
                return;

            u32 animIndex = player.animation_index;
            if(animIndex >= modelAss->modelData.animations.size()) {
                animIndex = 0;    // Fallback
            }
            const auto& animData = modelAss->modelData.animations[animIndex];

            // Convert to ticks（clip_start_time/clip_end_timeで指定されたレンジ内で評価する）
            float animTime = EvaluateClipTicks(animData, player.elapsed_time, player.clip_start_time,
                                               player.clip_end_time, player.is_looping, player.is_finished);

            // Outgoing anim blend logic: 遷移中は、フェードアウトしていくoutgoingクリップを
            // 自分自身の経過時間(controller->outgoing.elapsed_time)で独立して評価する
            float                                         finalBlendAlpha = 0.0f;
            const Tsukino::GraphicsCommon::AnimationData* blendAnimData   = nullptr;
            float                                         blendAnimTime   = 0.0f;

            if(controller && controller->is_transitioning && controller->outgoing.clip.IsValid()) {
                auto outAsset = ctx->assetManager->Get(controller->outgoing.clip);
                if(outAsset && outAsset->GetType() == Tsukino::Asset::AssetType::Model) {
                    auto outModelAss = std::static_pointer_cast<Tsukino::Asset::ModelAsset>(outAsset);
                    if(!outModelAss->modelData.animations.empty()) {
                        u32 outAnimIndex = controller->outgoing.animation_index;
                        if(outAnimIndex >= outModelAss->modelData.animations.size()) {
                            outAnimIndex = 0;
                        }
                        blendAnimData = &outModelAss->modelData.animations[outAnimIndex];

                        bool outFinished;    // outgoing側は既に再生完了扱いのため使わない（フェードアウト目的のみ）
                        blendAnimTime = EvaluateClipTicks(*blendAnimData, controller->outgoing.elapsed_time,
                                                          controller->outgoing.clip_start_time,
                                                          controller->outgoing.clip_end_time,
                                                          controller->outgoing.is_looping, outFinished);

                        finalBlendAlpha = controller->blend_alpha;
                    }
                }
            }

            //-------------------------------------------------------------
            // スキニングに使うnodes/skeleton.bonesの取得元を決める。
            //
            // クリップアセット（modelAss）自身のnodes/skeletonは使わない：Mixamo等から
            // 「アニメーションのみ」（メッシュ・スキン無し）でエクスポートしたFBXは
            // skeleton.bonesが空になる（ModelImporterがaiMesh->mBonesからしか
            // ボーン一覧を作れないため）。そのままではbone_count=0になり再生できない。
            //
            // 代わりに、エンティティ本体のModelComponentが指すキャラクターモデル
            // （描画に使われている実メッシュ）のnodes/skeletonを正とする。これは
            // クリップが何であってもボーン構成が変わらない安定した基準であり、
            // 描画側（ModelSystem）もこのモデルの頂点ボーンインデックスを使って
            // スキニングするため、本来こちらが正しい対応関係になる。
            // ModelComponentが無い/未解決の場合のみ、従来通りクリップアセット自身へ
            // フォールバックする（メッシュ入りクリップを使う既存シーンの互換のため）
            //-------------------------------------------------------------
            std::shared_ptr<Tsukino::Asset::ModelAsset> skeletonModelAss = modelAss;
            if(registry.HasComponent<ModelComponent>(entity)) {
                auto& modelComp = registry.GetComponent<ModelComponent>(entity);
                if(modelComp.modelHandle.IsValid()) {
                    auto charAsset = ctx->assetManager->Get(modelComp.modelHandle);
                    if(charAsset && charAsset->GetType() == Tsukino::Asset::AssetType::Model) {
                        skeletonModelAss = std::static_pointer_cast<Tsukino::Asset::ModelAsset>(charAsset);
                    }
                }
            }

            //-------------------------------------------------------------
            // In Place用：ルートモーションノードのindexを解決する（初回のみ、以後キャッシュ）。
            // ModelComponentのモデルを実行中に差し替えると古いままになる点はSpringBoneの
            // resolvedフラグと同じ制約
            //-------------------------------------------------------------
            if(!player.root_motion_resolved) {
                player.root_motion_node_index = ResolveRootMotionNode(skeletonModelAss->modelData, player.root_motion_node_name);
                player.root_motion_resolved   = true;
#if defined(_DEBUG)
                const std::string resolvedName = (player.root_motion_node_index < skeletonModelAss->modelData.nodes.size())
                                                      ? skeletonModelAss->modelData.nodes[player.root_motion_node_index].name
                                                      : "(none)";
                Tsukino::Core::Log::Info("AnimationSystem: root motion node resolved to index="
                                         + std::to_string(player.root_motion_node_index) + " (" + resolvedName + ")");
#endif
            }

            // In Place：現在クリップ／ブレンド元(outgoing)クリップのうち、どちらかがIn Place対象か
            bool applyInPlace      = player.in_place && player.root_motion_node_index != UINT32_MAX;
            bool applyBlendInPlace = controller && controller->outgoing.in_place && blendAnimData
                                     && player.root_motion_node_index != UINT32_MAX;
            bool needsRootLock = applyInPlace || applyBlendInPlace;

            // 「非In Place → In Place」に切り替わった最初のフレームでだけ、現在の生のHips位置を
            // 凍結基準として記録する。以後、In Placeが有効な間（連撃の各段のようにクリップが
            // 切り替わり続けても）はこの基準を使い続けるため、段ごとに元モーションの前進量ぶん
            // 位置がジャンプすることがない
            if(needsRootLock && !player.root_motion_lock_active) {
                const auto* srcChannels = applyInPlace ? &animData.channels : &blendAnimData->channels;
                const float srcTime     = applyInPlace ? animTime : blendAnimTime;
                for(const auto& channel : *srcChannels) {
                    if(channel.nodeName == skeletonModelAss->modelData.nodes[player.root_motion_node_index].name) {
                        hlslpp::float3 rawPos      = LerpVector(channel.positionKeys, srcTime);
                        player.root_motion_lock_x = rawPos.x;
                        player.root_motion_lock_z = rawPos.z;
                        break;
                    }
                }
            }
            player.root_motion_lock_active = needsRootLock;

            //-------------------------------------------------------------
            // ノード→チャンネルの対応表を引く（初回のみ構築、以後キャッシュ）。
            // 毎ノードで全チャンネルを文字列比較していた箇所を、この表の添字引きへ置き換える
            //-------------------------------------------------------------
            const std::vector<std::int32_t>& channelTable = ResolveChannelTable(animData, skeletonModelAss->modelData);

            const std::vector<std::int32_t>* blendChannelTable = nullptr;
            if(blendAnimData != nullptr)
                blendChannelTable = &ResolveChannelTable(*blendAnimData, skeletonModelAss->modelData);

            //-------------------------------------------------------------
            // 全ノードのグローバル行列を計算
            //-------------------------------------------------------------
            std::vector<Tsukino::Core::Math::matrix> globalNodeMatrices(skeletonModelAss->modelData.nodes.size());

            // 揺れ物物理用：位置・回転だけの軽量なワールド姿勢も並行して計算しておく
            // （スケールは1と仮定。揺れ物ボーンにスケールアニメを使わない前提の簡易版）
            std::vector<Tsukino::Physics::WorldPose> worldPoses(skeletonModelAss->modelData.nodes.size());

            // ノードは親から子の順に並んでいる前提（一般的なフォーマット）で計算
            for(size_t i = 0; i < skeletonModelAss->modelData.nodes.size(); ++i) {
                const auto& node = skeletonModelAss->modelData.nodes[i];

                hlslpp::float3     pos(node.translation.x, node.translation.y, node.translation.z);
                hlslpp::quaternion rot(node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w);
                hlslpp::float3     scale(node.scale.x, node.scale.y, node.scale.z);

                bool channelFound = false;

                // 現在のアニメーションチャンネルを対応表から引く
                const std::int32_t channelIndex = (i < channelTable.size()) ? channelTable[i] : -1;
                if(channelIndex >= 0) {
                    const auto& channel = animData.channels[static_cast<std::size_t>(channelIndex)];

                    pos          = LerpVector(channel.positionKeys, animTime);
                    rot          = SlerpQuaternion(channel.rotationKeys, animTime);
                    scale        = LerpVector(channel.scaleKeys, animTime);
                    channelFound = true;
                }

                // In Place：ルートノードの水平移動だけ固定する（Yはアニメのまま残す）
                if(applyInPlace && i == player.root_motion_node_index && channelFound) {
                    pos = hlslpp::float3(player.root_motion_lock_x, pos.y, player.root_motion_lock_z);
                }

                // ブレンド処理の適用
                if(finalBlendAlpha > 0.0f && blendAnimData) {
                    hlslpp::float3     blendPos(node.translation.x, node.translation.y, node.translation.z);
                    hlslpp::quaternion blendRot(node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w);
                    hlslpp::float3     blendScale(node.scale.x, node.scale.y, node.scale.z);
                    bool               blendChannelFound = false;

                    // ブレンド元も同じく対応表から引く
                    const std::int32_t blendChannelIndex =
                        (blendChannelTable != nullptr && i < blendChannelTable->size()) ? (*blendChannelTable)[i] : -1;
                    if(blendChannelIndex >= 0) {
                        const auto& bChannel = blendAnimData->channels[static_cast<std::size_t>(blendChannelIndex)];

                        blendPos          = LerpVector(bChannel.positionKeys, blendAnimTime);
                        blendRot          = SlerpQuaternion(bChannel.rotationKeys, blendAnimTime);
                        blendScale        = LerpVector(bChannel.scaleKeys, blendAnimTime);
                        blendChannelFound = true;
                    }

                    // In Place：ブレンド元（outgoing）側のルートノードも、current側と同じ凍結基準で固定する。
                    // これをしないとクロスフェード中だけ移動が復活したり、段ごとの基準ズレでジャンプする
                    if(applyBlendInPlace && i == player.root_motion_node_index && blendChannelFound) {
                        blendPos = hlslpp::float3(player.root_motion_lock_x, blendPos.y, player.root_motion_lock_z);
                    }

                    if(channelFound || blendChannelFound) {
                        // クォータニオンは二重被覆（qと-qが同じ回転）のため、内積が負なら
                        // 片方を反転して最短経路でslerpする（SlerpQuaternion内の処理と同じ理由）。
                        // これをしないと関節が遠回りの経路で回転し、膝などが不自然に曲がって見える
                        float dot = blendRot.x * rot.x + blendRot.y * rot.y + blendRot.z * rot.z + blendRot.w * rot.w;
                        if(dot < 0.0f) {
                            rot = hlslpp::quaternion(-rot.x, -rot.y, -rot.z, -rot.w);
                        }

                        // pos/rot/scale = 遷移先(新)クリップ、blendPos/blendRot/blendScale = 遷移元(旧)クリップ。
                        // finalBlendAlphaは0(旧のまま)→1(新のまま)へ進むので、旧を起点にlerpする
                        pos   = hlslpp::lerp(blendPos, pos, finalBlendAlpha);
                        rot   = hlslpp::slerp(blendRot, rot, finalBlendAlpha);
                        scale = hlslpp::lerp(blendScale, scale, finalBlendAlpha);
                    }
                }

                // ローカル行列の計算
                // (行優先 / Column-major等の仕様に合わせ、T * R * S とするケースに対応)
                Tsukino::Core::Math::matrix scaleMat = Tsukino::Core::Math::matrix::scale(scale);
                Tsukino::Core::Math::matrix rotMat   = Tsukino::Core::Math::matrix::rotate(rot);
                Tsukino::Core::Math::matrix transMat = Tsukino::Core::Math::matrix::translate(pos);
                // ご指摘の通り親を左側に乗算する仕様 (Parent * Local) の場合、SRTの順序も T * R * S であるべきケースが多いです
                Tsukino::Core::Math::matrix localMat = hlslpp::mul(scaleMat, hlslpp::mul(rotMat, transMat));

                // グローバル（ワールド）行列の算出
                if(node.parentIndex != UINT32_MAX && node.parentIndex < globalNodeMatrices.size()) {
                    // 親が左、ローカルが右（Parent * Local）になるように修正
                    globalNodeMatrices[i] = hlslpp::mul(localMat, globalNodeMatrices[node.parentIndex]);
                } else {
                    // ルートノードの場合はローカル行列がそのままグローバル行列
                    globalNodeMatrices[i] = localMat;
                }

                // ワールド姿勢（位置・回転のみ）も同じ合成（子のローカルが先、親が後）で並行計算。
                //
                // 【重要】hlsl++は行列とクォータニオンで合成順の規約が逆。
                //   「ローカルを適用してから親を適用する」合成は、
                //     行列          : hlslpp::mul(localMat, parentWorld)  （上のglobalNodeMatricesと同じ）
                //     クォータニオン: hlslpp::mul(parentRot, localRot)    （Hamilton積。親を左に置く）
                //   （hlslpp本体のunit_tests_quaternion.cpp「M_AB = M_A * M_B / Q_AB = Q_B * Q_A」参照）
                //   ベクトルの回転は mul(v, q) が前方回転（mul(q, v) は逆回転の別関数）
                const bool hasParent = (node.parentIndex != UINT32_MAX && node.parentIndex < worldPoses.size());
                if(hasParent) {
                    const auto& parentPose = worldPoses[node.parentIndex];

                    worldPoses[i].rotation = hlslpp::mul(parentPose.rotation, rot);
                    worldPoses[i].position = parentPose.position + hlslpp::mul(pos, parentPose.rotation);
                } else {
                    worldPoses[i].rotation = rot;
                    worldPoses[i].position = pos;
                }
            }

            //-------------------------------------------------------------
            // 他エンティティ（武器のボーンアタッチ等）から参照できるよう、
            // 各ノードのワールド姿勢（モデルローカル空間）を公開する
            //-------------------------------------------------------------
            {
                auto& poseOut = registry.HasComponent<NodeWorldPoseComponent>(entity)
                                    ? registry.GetComponent<NodeWorldPoseComponent>(entity)
                                    : registry.AddComponent<NodeWorldPoseComponent>(entity);
                poseOut.poses = worldPoses;
            }

            //-------------------------------------------------------------
            // 揺れ物物理（SpringBone）：対象ノードのglobalNodeMatricesを上書き
            //-------------------------------------------------------------
            if(registry.HasComponent<SpringBoneComponent>(entity)) {
                auto& springBone = registry.GetComponent<SpringBoneComponent>(entity);

                if(springBone.enabled) {
                    // 初回だけ：ノード名を解決してチェーンを構築する
                    if(!springBone.resolved) {
                        std::unordered_map<std::string, u32> nameToIndex;
                        for(u32 i = 0; i < skeletonModelAss->modelData.nodes.size(); ++i) {
                            nameToIndex[skeletonModelAss->modelData.nodes[i].name] = i;
                        }

                        springBone.chains.clear();

                        for(const auto& def : springBone.chainDefs) {
                            std::unordered_set<std::string> excludeSet(def.excludeNodeNames.begin(), def.excludeNodeNames.end());

                            Tsukino::Physics::SpringBoneChain chain;

                            if(!def.rootNodeName.empty()) {
                                // 特定の1本から始める（兄弟を巻き込まない）
                                auto rootIt = nameToIndex.find(def.rootNodeName);
                                if(rootIt == nameToIndex.end()) {
                                    Tsukino::Core::Log::Error("SpringBone: root node not found: " + def.rootNodeName);
                                    continue;
                                }
                                chain = Tsukino::Physics::SpringBonePhysics::BuildChainFromRoot(
                                    def.name, rootIt->second, skeletonModelAss->modelData.nodes, excludeSet, def.maxDepth, def.settings);
                            } else {
                                // アンカーの子孫を全部揺らす（髪など）
                                auto anchorIt = nameToIndex.find(def.anchorNodeName);
                                if(anchorIt == nameToIndex.end()) {
                                    Tsukino::Core::Log::Error("SpringBone: anchor node not found: " + def.anchorNodeName);
                                    continue;
                                }
                                chain = Tsukino::Physics::SpringBonePhysics::BuildChainFromHierarchy(
                                    def.name, anchorIt->second, skeletonModelAss->modelData.nodes, excludeSet, def.maxDepth, def.settings);
                            }

                            for(const auto& colliderDef : def.colliders) {
                                Tsukino::Physics::SpringColliderSphere collider;
                                collider.localOffset = colliderDef.localOffset;
                                collider.radius      = colliderDef.radius;
                                if(!colliderDef.attachNodeName.empty()) {
                                    auto it                  = nameToIndex.find(colliderDef.attachNodeName);
                                    collider.attachNodeIndex = (it != nameToIndex.end()) ? it->second : UINT32_MAX;
                                }
                                chain.colliders.push_back(collider);
                            }

                            springBone.chains.push_back(std::move(chain));
                        }

                        for(auto& chain : springBone.chains) {
                            Tsukino::Physics::SpringBonePhysics::InitializeChain(chain, worldPoses);
                        }

                        springBone.resolved = true;

                        //---------------------------------------------------------
                        // 【一度だけ】チェーン構築結果のサマリ。
                        // ここで各ノードの実名・親・restLengthが分かる。
                        //---------------------------------------------------------
                        for(const auto& chain : springBone.chains) {
                            const std::string anchorName =
                                (chain.anchorNodeIndex < skeletonModelAss->modelData.nodes.size()) ? skeletonModelAss->modelData.nodes[chain.anchorNodeIndex].name : "(none)";
                            Tsukino::Core::Log::Info("SpringBone chain '" + chain.name + "' resolved: " + std::to_string(chain.nodes.size())
                                                     + " nodes, anchor=" + std::to_string(chain.anchorNodeIndex) + " (" + anchorName + ")");
                            for(const auto& n : chain.nodes) {
                                const auto& nd = skeletonModelAss->modelData.nodes[n.nodeIndex];
                                Tsukino::Core::Log::Info("  node=" + std::to_string(n.nodeIndex) + " (" + nd.name + ")"
                                                         + " parentIndex=" + std::to_string(nd.parentIndex) + " restLength=" + std::to_string(n.restLength));
                            }
                        }
                    }

                    // 毎フレーム更新して、対象ノードのglobalNodeMatricesを上書き
                    for(auto& chain : springBone.chains) {
                        Tsukino::Physics::SpringBonePhysics::UpdateChain(chain, deltaTime, worldPoses);

                        for(const auto& node : chain.nodes) {
                            if(node.nodeIndex >= globalNodeMatrices.size())
                                continue;

                            Tsukino::Core::Math::matrix rotMat   = Tsukino::Core::Math::matrix::rotate(node.correctedRotation);
                            Tsukino::Core::Math::matrix transMat = Tsukino::Core::Math::matrix::translate(node.currentPosition);
                            globalNodeMatrices[node.nodeIndex]   = hlslpp::mul(rotMat, transMat);
                        }
                    }

                    //---------------------------------------------------------
                    // 【毎フレーム、間引き】Jiggle(揺れ)の数値デバッグ。
                    // - pos          : 物理適用後の実座標(world)。これが時間で
                    //                  変化していれば「動いている」証拠。
                    // - displacement : アニメだけの位置との差。0でなければ
                    //                  物理が何かしている証拠。
                    // - このログを2〜3秒分並べて見た時、displacementが
                    //   一方向に増え続けるなら「発散」、上下に振れているなら
                    //   「揺れ(jiggle)」、ずっと同じ値なら「静止して釣り合っている」。
                    //---------------------------------------------------------
                    static float debugTimer  = 0.0f;
                    debugTimer              += deltaTime;
                    if(debugTimer > 0.2f) {
                        debugTimer = 0.0f;
                        for(const auto& chain : springBone.chains) {
                            for(const auto& node : chain.nodes) {
                                if(node.nodeIndex >= worldPoses.size())
                                    continue;

                                const hlslpp::float3 animOnlyPos = worldPoses[node.nodeIndex].position;
                                const hlslpp::float3 physicsPos  = node.currentPosition;
                                const hlslpp::float3 diff        = physicsPos - animOnlyPos;
                                const float          diffLen     = float(hlslpp::length(diff));

                                Tsukino::Core::Log::Info("JIGGLE '" + chain.name + "' node=" + std::to_string(node.nodeIndex) + " pos=("
                                                         + std::to_string(physicsPos.x) + ", " + std::to_string(physicsPos.y) + ", "
                                                         + std::to_string(physicsPos.z) + ")" + " displacement=" + std::to_string(diffLen));
                            }
                        }
                    }
                }
            }

            //-------------------------------------------------------------
            // 他エンティティ（武器のボーンソケットアタッチ等）から参照できるよう、
            // 各ノードのスケール込みグローバル行列を公開する（揺れ物補正後の最終値。
            // これがそのままスキニングにも使われるため、見た目と完全に一致する）
            //-------------------------------------------------------------
            {
                auto& matrixOut = registry.HasComponent<NodeWorldMatrixComponent>(entity)
                                       ? registry.GetComponent<NodeWorldMatrixComponent>(entity)
                                       : registry.AddComponent<NodeWorldMatrixComponent>(entity);
                matrixOut.matrices = globalNodeMatrices;
            }

            //-------------------------------------------------------------
            // ボーン行列の計算
            //-------------------------------------------------------------
            skeletonOut.bone_count = 0;
            for(u32 idx = 0; idx < skeletonModelAss->modelData.skeleton.bones.size() && idx < SkeletonOutputComponent::MAX_BONES; ++idx) {
                const auto& boneInfo = skeletonModelAss->modelData.skeleton.bones[idx];

                Tsukino::Core::Math::matrix globalNodeMat = Tsukino::Core::Math::matrix::identity();

                // ボーンに対応するノードのグローバル行列を取得
                if(boneInfo.nodeIndex < globalNodeMatrices.size()) {
                    globalNodeMat = globalNodeMatrices[boneInfo.nodeIndex];
                }

                // スキニング行列（Global Node Matrix * Inverse Bind Pose）になるよう乗算順序を修正
                Tsukino::Core::Math::matrix finalBoneMat = hlslpp::mul(boneInfo.inverseBindPose, globalNodeMat);

                // SkeletonOutput に書き出し
                std::memcpy(skeletonOut.local_matrices[idx], &finalBoneMat, sizeof(float) * 16);
                skeletonOut.bone_count++;
            }
        });
    }

}    // namespace Tsukino::BuiltIn::ECS

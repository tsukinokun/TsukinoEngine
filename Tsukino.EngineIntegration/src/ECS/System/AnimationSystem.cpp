//-------------------------------------------------------------
//! @file   AnimationSystem.cpp
//! @brief  AnimationSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/AnimationSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp>
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
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace Tsukino::BuiltIn::ECS {

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

                    player.current_clip_id = controller->next.clip;
                    player.animation_index = controller->next.animation_index;
                    player.elapsed_time    = 0.0f;
                    player.is_looping      = controller->next.is_looping;

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

            // Convert to ticks
            float ticks    = player.elapsed_time * animData.ticksPerSecond;
            float animTime = std::fmod(ticks, animData.duration);
            if(!player.is_looping && ticks >= animData.duration) {
                animTime           = animData.duration;
                player.is_finished = true;
            } else {
                player.is_finished = false;
            }

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

                        float outTicks = controller->outgoing.elapsed_time * blendAnimData->ticksPerSecond;
                        blendAnimTime  = std::fmod(outTicks, blendAnimData->duration);
                        if(!controller->outgoing.is_looping && outTicks >= blendAnimData->duration) {
                            blendAnimTime = blendAnimData->duration;
                        }

                        finalBlendAlpha = controller->blend_alpha;
                    }
                }
            }

            //-------------------------------------------------------------
            // 全ノードのグローバル行列を計算
            //-------------------------------------------------------------
            std::vector<Tsukino::Core::Math::matrix> globalNodeMatrices(modelAss->modelData.nodes.size());

            // 揺れ物物理用：位置・回転だけの軽量なワールド姿勢も並行して計算しておく
            // （スケールは1と仮定。揺れ物ボーンにスケールアニメを使わない前提の簡易版）
            std::vector<Tsukino::Physics::WorldPose> worldPoses(modelAss->modelData.nodes.size());

            // ノードは親から子の順に並んでいる前提（一般的なフォーマット）で計算
            for(size_t i = 0; i < modelAss->modelData.nodes.size(); ++i) {
                const auto& node = modelAss->modelData.nodes[i];

                hlslpp::float3     pos(node.translation.x, node.translation.y, node.translation.z);
                hlslpp::quaternion rot(node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w);
                hlslpp::float3     scale(node.scale.x, node.scale.y, node.scale.z);

                bool channelFound = false;

                // 現在のアニメーションチャンネルを検索
                for(const auto& channel : animData.channels) {
                    if(channel.nodeName == node.name) {
                        pos          = LerpVector(channel.positionKeys, animTime);
                        rot          = SlerpQuaternion(channel.rotationKeys, animTime);
                        scale        = LerpVector(channel.scaleKeys, animTime);
                        channelFound = true;
                        break;
                    }
                }

                // ブレンド処理の適用
                if(finalBlendAlpha > 0.0f && blendAnimData) {
                    hlslpp::float3     blendPos(node.translation.x, node.translation.y, node.translation.z);
                    hlslpp::quaternion blendRot(node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w);
                    hlslpp::float3     blendScale(node.scale.x, node.scale.y, node.scale.z);
                    bool               blendChannelFound = false;

                    for(const auto& bChannel : blendAnimData->channels) {
                        if(bChannel.nodeName == node.name) {
                            blendPos          = LerpVector(bChannel.positionKeys, blendAnimTime);
                            blendRot          = SlerpQuaternion(bChannel.rotationKeys, blendAnimTime);
                            blendScale        = LerpVector(bChannel.scaleKeys, blendAnimTime);
                            blendChannelFound = true;
                            break;
                        }
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

                // ワールド姿勢（位置・回転のみ）も同じ合成順（子のローカルが先、親が後）で並行計算
                // mul(A,B) は「Aを適用してからBを適用する」規則（globalNodeMatricesの
                //   mul(localMat, parentWorld)と同じ規則）に合わせてある
                const bool hasParent = (node.parentIndex != UINT32_MAX && node.parentIndex < worldPoses.size());
                if(hasParent) {
                    const auto& parentPose = worldPoses[node.parentIndex];

                    worldPoses[i].rotation = hlslpp::mul(rot, parentPose.rotation);
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
                        for(u32 i = 0; i < modelAss->modelData.nodes.size(); ++i) {
                            nameToIndex[modelAss->modelData.nodes[i].name] = i;
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
                                    def.name, rootIt->second, modelAss->modelData.nodes, excludeSet, def.maxDepth, def.settings);
                            } else {
                                // アンカーの子孫を全部揺らす（髪など）
                                auto anchorIt = nameToIndex.find(def.anchorNodeName);
                                if(anchorIt == nameToIndex.end()) {
                                    Tsukino::Core::Log::Error("SpringBone: anchor node not found: " + def.anchorNodeName);
                                    continue;
                                }
                                chain = Tsukino::Physics::SpringBonePhysics::BuildChainFromHierarchy(
                                    def.name, anchorIt->second, modelAss->modelData.nodes, excludeSet, def.maxDepth, def.settings);
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
                                (chain.anchorNodeIndex < modelAss->modelData.nodes.size()) ? modelAss->modelData.nodes[chain.anchorNodeIndex].name : "(none)";
                            Tsukino::Core::Log::Info("SpringBone chain '" + chain.name + "' resolved: " + std::to_string(chain.nodes.size())
                                                     + " nodes, anchor=" + std::to_string(chain.anchorNodeIndex) + " (" + anchorName + ")");
                            for(const auto& n : chain.nodes) {
                                const auto& nd = modelAss->modelData.nodes[n.nodeIndex];
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
            for(u32 idx = 0; idx < modelAss->modelData.skeleton.bones.size() && idx < SkeletonOutputComponent::MAX_BONES; ++idx) {
                const auto& boneInfo = modelAss->modelData.skeleton.bones[idx];

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

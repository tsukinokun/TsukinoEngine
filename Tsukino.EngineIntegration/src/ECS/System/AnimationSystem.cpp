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

            if(controller) {
                if(controller->next.clip.IsValid()) {
                    if(controller->next.immediate) {
                        player.current_clip_id       = controller->next.clip;
                        player.animation_index       = controller->next.animation_index;
                        player.elapsed_time          = 0.0f;
                        controller->next.clip        = Tsukino::Asset::AssetHandle{};
                        controller->is_transitioning = false;
                        controller->blend_alpha      = 0.0f;
                    } else {
                        // Blend logic implementation
                        // [Simplified] for demonstration
                    }
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
                animTime = animData.duration;
            }

            // Next anim blend logic
            float                                         finalBlendAlpha = 0.0f;
            const Tsukino::GraphicsCommon::AnimationData* blendAnimData   = nullptr;
            float                                         blendAnimTime   = 0.0f;

            AnimationControllerComponent* pController = nullptr;
            if(registry.HasComponent<AnimationControllerComponent>(entity)) {
                pController = &registry.GetComponent<AnimationControllerComponent>(entity);
            }

            if(pController && pController->is_transitioning && pController->next.clip.IsValid()) {
                auto nextAsset = ctx->assetManager->Get(pController->next.clip);
                if(nextAsset && nextAsset->GetType() == Tsukino::Asset::AssetType::Model) {
                    auto nextModelAss = std::static_pointer_cast<Tsukino::Asset::ModelAsset>(nextAsset);
                    if(!nextModelAss->modelData.animations.empty()) {
                        u32 nextAnimIndex = pController->next.animation_index;
                        if(nextAnimIndex >= nextModelAss->modelData.animations.size()) {
                            nextAnimIndex = 0;
                        }
                        blendAnimData             = &nextModelAss->modelData.animations[nextAnimIndex];
                        pController->blend_alpha += deltaTime / pController->next.fade_time;
                        if(pController->blend_alpha >= 1.0f) {
                            pController->blend_alpha      = 1.0f;
                            pController->is_transitioning = false;
                            player.current_clip_id        = pController->next.clip;
                            player.animation_index        = pController->next.animation_index;
                            player.elapsed_time           = 0.0f;    // Simplified, in reality would have to keep track of both times
                            pController->next.clip        = Tsukino::Asset::AssetHandle{};
                        }
                        finalBlendAlpha = pController->blend_alpha;

                        // we need elapsed time of the next animation, let's just make it 0 for now for simplification or use controller fade progress
                        float blendTicks = (pController->blend_alpha * pController->next.fade_time) * blendAnimData->ticksPerSecond;
                        blendAnimTime    = std::fmod(blendTicks, blendAnimData->duration);
                    }
                }
            } else if(pController && !pController->is_transitioning && pController->next.clip.IsValid() && !pController->next.immediate) {
                if(!player.is_looping && ticks >= animData.duration) {
                    pController->is_transitioning = true;
                    pController->blend_alpha      = 0.0f;
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
                        pos   = hlslpp::lerp(pos, blendPos, finalBlendAlpha);
                        rot   = hlslpp::slerp(rot, blendRot, finalBlendAlpha);
                        scale = hlslpp::lerp(scale, blendScale, finalBlendAlpha);
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
                // ※ mul(A,B) は「Aを適用してからBを適用する」規則（globalNodeMatricesの
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
                            auto anchorIt = nameToIndex.find(def.anchorNodeName);
                            if(anchorIt == nameToIndex.end()) {
                                Tsukino::Core::Log::Error("SpringBone: anchor node not found: " + def.anchorNodeName);
                                continue;
                            }

                            std::unordered_set<std::string> excludeSet(def.excludeNodeNames.begin(), def.excludeNodeNames.end());

                            auto chain = Tsukino::Physics::SpringBonePhysics::BuildChainFromHierarchy(
                                def.name, anchorIt->second, modelAss->modelData.nodes, excludeSet, def.maxDepth, def.settings);

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
                }
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

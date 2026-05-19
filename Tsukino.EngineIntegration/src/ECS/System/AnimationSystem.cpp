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
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Model/ModelAsset.hpp>
#include <Tsukino/GraphicsCommon/Model/ModelData.hpp>

#include <entt/entt.hpp>
#include <cmath>

namespace Tsukino::BuiltIn::ECS {

    //-------------------------------------------------------------
    //! @brief ベクトルの線形補間（位置・スケール用）
    //! @param keys キーフレームのリスト
    //! @param time 現在のアニメーション時間（Ticks）
    //! @return 補間されたhlslpp::float3
    //-------------------------------------------------------------
    static hlslpp::float3 LerpVector(const std::vector<Tsukino::GraphicsCommon::VectorKey>& keys, float time) {
        if (keys.empty()) return hlslpp::float3(0, 0, 0);
        if (keys.size() == 1 || time <= keys.front().time) return hlslpp::float3(keys.front().value.x, keys.front().value.y, keys.front().value.z);
        if (time >= keys.back().time) return hlslpp::float3(keys.back().value.x, keys.back().value.y, keys.back().value.z);

        for (size_t i = 0; i < keys.size() - 1; ++i) {
            if (time >= keys[i].time && time < keys[i + 1].time) {
                float t = (time - keys[i].time) / (keys[i + 1].time - keys[i].time);
                return hlslpp::lerp(hlslpp::float3(keys[i].value.x, keys[i].value.y, keys[i].value.z), hlslpp::float3(keys[i + 1].value.x, keys[i+1].value.y, keys[i+1].value.z), hlslpp::float3(t, t, t));
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
        if (keys.empty()) return hlslpp::quaternion(0, 0, 0, 1);
        if (keys.size() == 1 || time <= keys.front().time) return hlslpp::quaternion(keys.front().value.x, keys.front().value.y, keys.front().value.z, keys.front().value.w);
        if (time >= keys.back().time) return hlslpp::quaternion(keys.back().value.x, keys.back().value.y, keys.back().value.z, keys.back().value.w);

        for (size_t i = 0; i < keys.size() - 1; ++i) {
            if (time >= keys[i].time && time < keys[i + 1].time) {
                float t = (time - keys[i].time) / (keys[i + 1].time - keys[i].time);
                hlslpp::quaternion q1 = hlslpp::quaternion(keys[i].value.x, keys[i].value.y, keys[i].value.z, keys[i].value.w);
                hlslpp::quaternion q2 = hlslpp::quaternion(keys[i+1].value.x, keys[i+1].value.y, keys[i+1].value.z, keys[i+1].value.w);
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
        if (!ctx || !ctx->assetManager) return;

        auto view = registry.View<AnimationPlayerComponent, SkeletonOutputComponent>();

        view.each([&](entt::entity entity, AnimationPlayerComponent& player, SkeletonOutputComponent& skeletonOut) {
            if (player.is_playing) {
                player.elapsed_time += deltaTime * player.playback_speed;
            }

            AnimationControllerComponent* controller = nullptr;
            if (registry.HasComponent<AnimationControllerComponent>(entity)) {
                controller = &registry.GetComponent<AnimationControllerComponent>(entity);
            }

            if (controller) {
                if (controller->next.clip.IsValid()) {
                    if (controller->next.immediate) {
                        player.current_clip_id = controller->next.clip;
                        player.elapsed_time = 0.0f;
                        controller->next.clip = Tsukino::Asset::AssetHandle{};
                        controller->is_transitioning = false;
                        controller->blend_alpha = 0.0f;
                    } else {
                        // Blend logic implementation
                        // [Simplified] for demonstration
                    }
                }
            }

            if (!player.current_clip_id.IsValid()) return;

            auto animAsset = ctx->assetManager->Get(player.current_clip_id);
            if (!animAsset || animAsset->GetType() != Tsukino::Asset::AssetType::Model) return;

            auto modelAss = std::static_pointer_cast<Tsukino::Asset::ModelAsset>(animAsset);
            if (modelAss->modelData.animations.empty()) return;

            const auto& animData = modelAss->modelData.animations[0];
            
            // Convert to ticks
            float ticks = player.elapsed_time * animData.ticksPerSecond;
            float animTime = std::fmod(ticks, animData.duration);
            if (!player.is_looping && ticks >= animData.duration) {
                animTime = animData.duration;
            }

            // Next anim blend logic
            float finalBlendAlpha = 0.0f;
            const Tsukino::GraphicsCommon::AnimationData* blendAnimData = nullptr;
            float blendAnimTime = 0.0f;
            
            AnimationControllerComponent* pController = nullptr;
            if (registry.HasComponent<AnimationControllerComponent>(entity)) {
                pController = &registry.GetComponent<AnimationControllerComponent>(entity);
            }

            if (pController && pController->is_transitioning && pController->next.clip.IsValid()) {
                auto nextAsset = ctx->assetManager->Get(pController->next.clip);
                if (nextAsset && nextAsset->GetType() == Tsukino::Asset::AssetType::Model) {
                    auto nextModelAss = std::static_pointer_cast<Tsukino::Asset::ModelAsset>(nextAsset);
                    if (!nextModelAss->modelData.animations.empty()) {
                        blendAnimData = &nextModelAss->modelData.animations[0];
                        pController->blend_alpha += deltaTime / pController->next.fade_time;
                        if (pController->blend_alpha >= 1.0f) {
                            pController->blend_alpha = 1.0f;
                            pController->is_transitioning = false;
                            player.current_clip_id = pController->next.clip;
                            player.elapsed_time = 0.0f; // Simplified, in reality would have to keep track of both times
                            pController->next.clip = Tsukino::Asset::AssetHandle{};
                        }
                        finalBlendAlpha = pController->blend_alpha;
                        
                        // we need elapsed time of the next animation, let's just make it 0 for now for simplification or use controller fade progress
                        float blendTicks = (pController->blend_alpha * pController->next.fade_time) * blendAnimData->ticksPerSecond;
                        blendAnimTime = std::fmod(blendTicks, blendAnimData->duration);
                    }
                }
            } else if (pController && !pController->is_transitioning && pController->next.clip.IsValid() && !pController->next.immediate) {
                 if (!player.is_looping && ticks >= animData.duration) {
                     pController->is_transitioning = true;
                     pController->blend_alpha = 0.0f;
                 }
            }

            // Calculate matrices
            skeletonOut.bone_count = 0;
            // Iterate over bones in modelAsset to find correct animation channels and calc matrix
            for (u32 idx = 0; idx < modelAss->modelData.skeleton.bones.size() && idx < SkeletonOutputComponent::MAX_BONES; ++idx) {
                const auto& boneInfo = modelAss->modelData.skeleton.bones[idx];
                
                hlslpp::float3 pos(0,0,0);
                hlslpp::quaternion rot(0,0,0,1);
                hlslpp::float3 scale(1,1,1);
                bool channelFound = false;

                for(const auto& channel : animData.channels) {
                     if (channel.nodeName == boneInfo.name) {
                         pos = LerpVector(channel.positionKeys, animTime);
                         rot = SlerpQuaternion(channel.rotationKeys, animTime);
                         scale = LerpVector(channel.scaleKeys, animTime);
                         channelFound = true;
                         break;
                     }
                }

                // If no channel found, use default node transform (not fully correct hierarchical logic, but simplifed here for bone matrix output)
                if (!channelFound) {
                    // Fallback
                }
                
                if (finalBlendAlpha > 0.0f && blendAnimData) {
                    hlslpp::float3 blendPos(0,0,0);
                    hlslpp::quaternion blendRot(0,0,0,1);
                    hlslpp::float3 blendScale(1,1,1);
                    bool blendChannelFound = false;
                    for(const auto& bChannel : blendAnimData->channels) {
                         if (bChannel.nodeName == boneInfo.name) {
                             blendPos = LerpVector(bChannel.positionKeys, blendAnimTime);
                             blendRot = SlerpQuaternion(bChannel.rotationKeys, blendAnimTime);
                             blendScale = LerpVector(bChannel.scaleKeys, blendAnimTime);
                             blendChannelFound = true;
                             break;
                         }
                    }
                    if (blendChannelFound) {
                        pos = hlslpp::lerp(pos, blendPos, finalBlendAlpha);
                        rot = hlslpp::slerp(rot, blendRot, finalBlendAlpha);
                        scale = hlslpp::lerp(scale, blendScale, finalBlendAlpha);
                    }
                }
                
                Tsukino::Core::Math::matrix scaleMat = Tsukino::Core::Math::matrix::scale(scale);
                Tsukino::Core::Math::matrix rotMat = Tsukino::Core::Math::matrix::rotate(rot);
                Tsukino::Core::Math::matrix transMat = Tsukino::Core::Math::matrix::translate(pos);
                Tsukino::Core::Math::matrix localMat = hlslpp::mul(hlslpp::mul(scaleMat, rotMat), transMat);
                
                // Usually we need to traverse hierarchy and mult with parent transforms, then inverseBindPose
                Tsukino::Core::Math::matrix finalBoneMat = hlslpp::mul(boneInfo.inverseBindPose, localMat);

                // Write to skeletonOut
                std::memcpy(skeletonOut.local_matrices[idx], &finalBoneMat, sizeof(float)*16);
                skeletonOut.bone_count++;
            }
        });
    }

}    // namespace Tsukino::BuiltIn::ECS

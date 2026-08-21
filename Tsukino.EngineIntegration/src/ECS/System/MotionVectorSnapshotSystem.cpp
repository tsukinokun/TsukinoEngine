//--------------------------------------------------------------
//! @file   MotionVectorSnapshotSystem.cpp
//! @brief  前フレームのトランスフォーム／ボーン行列を退避するシステムの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/MotionVectorSnapshotSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/MotionVectorComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkeletonOutputComponent.hpp>

#include <entt/entt.hpp>

#include <algorithm>
#include <cstring>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief システムの更新
    //--------------------------------------------------------------
    void MotionVectorSnapshotSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        (void)deltaTime;

        auto view = registry.View<MotionVectorComponent, TransformComponent>();

        view.each([&](entt::entity entity, MotionVectorComponent& motionVec, const TransformComponent& transform) {
            //------------------------------------------------------
            // ワールド行列の退避
            // TransformSystem がまだ走っていないので、ここで読める
            // worldMatrix は前フレームの値
            //------------------------------------------------------
            motionVec.prevWorld = transform.worldMatrix;

            //------------------------------------------------------
            // ボーン行列の退避（スキンメッシュのみ）
            // AnimationSystem がまだ走っていないので、ここで読める
            // local_matrices も前フレームの値
            //------------------------------------------------------
            auto* skeletonOut = registry.try_get<SkeletonOutputComponent>(entity);
            if(skeletonOut && skeletonOut->bone_count > 0) {
                const u32 copyCount = std::min<u32>(skeletonOut->bone_count, MotionVectorComponent::MAX_BONES);
                std::memcpy(motionVec.prevBones, skeletonOut->local_matrices, sizeof(float) * 16 * copyCount);
                motionVec.prevBoneCount = copyCount;
            } else {
                motionVec.prevBoneCount = 0;
            }

            //------------------------------------------------------
            // 1回でも退避できたら以降は有効
            // （アタッチされた最初のフレームだけ速度ゼロになる）
            //------------------------------------------------------
            motionVec.valid = true;
        });
    }

}    // namespace Tsukino::BuiltIn::ECS

//--------------------------------------------------------------
//! @file   BoneFollowColliderSystem.cpp
//! @brief  BoneFollowColliderSystemクラスの実装
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/BoneFollowColliderSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/BoneLookup.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/BoneFollowColliderComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/NodeWorldMatrixComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>

#include <hlsl++.h>
#include <entt/entt.hpp>

#include <cmath>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @brief システムの更新
    //--------------------------------------------------------------
    void BoneFollowColliderSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();

        auto view = registry.View<BoneFollowColliderComponent, CollisionComponent, TransformComponent>();
        view.each([&](entt::entity entity, BoneFollowColliderComponent& follow, CollisionComponent& collision, TransformComponent& transform) {
            ResolveBoneNodeIndex(ctx, registry, entity, follow.boneName, follow.resolvedAgainstModel, follow.boneNodeIndex);
            const auto* matrices = registry.try_get<NodeWorldMatrixComponent>(entity);
            if(follow.boneNodeIndex == UINT32_MAX || !matrices || follow.boneNodeIndex >= matrices->matrices.size())
                return;

            // スケール込みのボーン行列（描画に使うもの）から位置だけ取る（モデル空間）。
            // NodeWorldPoseComponentはスケール1近似の軽量版なので使わない
            hlslpp::float3     bonePosition;
            hlslpp::quaternion boneRotation;
            Tsukino::Core::Math::matrix::decomposePositionRotation(matrices->matrices[follow.boneNodeIndex], bonePosition, boneRotation);

            // 基準は最初に取れたフレーム。そのときのoffsetPositionも覚え、以後はボーンが動いた分だけ足す
            if(!follow.hasRest) {
                follow.baseOffset       = collision.offsetPosition;
                follow.restBonePosition = bonePosition;
                follow.hasRest          = true;
            }

            // offsetPositionはPhysicsSystemが向きで回す（スケールは掛けない）ので、モデル空間のまま渡しスケールだけここで掛ける
            hlslpp::float3 shift = (bonePosition - follow.restBonePosition) * transform.scale;
            if(!follow.followHeight)
                shift.y = 0.0f;

            const float lerpT        = follow.followSpeed > 0.0f ? 1.0f - std::exp(-follow.followSpeed * deltaTime) : 1.0f;
            follow.currentShift      = follow.currentShift + (shift - follow.currentShift) * lerpT;
            collision.offsetPosition = follow.baseOffset + follow.currentShift;
        });
    }
}    // namespace Tsukino::BuiltIn::ECS

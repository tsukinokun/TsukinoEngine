//-------------------------------------------------------------
//! @file   PlatformSystem.cpp
//! @brief  PlatformSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/JumpGameSample/ECS/System/PlatformSystem.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/PlatformComponent.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/LandedOnPlatformComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>

#include <hlsl++.h>
// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void PlatformSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // コンテキストの取得
        //-------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx)
            return;

        Tsukino::Input::InputSystem* inputSystem = ctx->inputSystem;

        //-------------------------------------------------------------
        // プレイヤーがこの土台に止まっているかを確認、乗ってるなら土台の動きを止める
        //-------------------------------------------------------------
        auto playerView = registry.View<LandedOnPlatformComponent, Tsukino::BuiltIn::ECS::RigidbodyComponent>();
        playerView.each([&](entt::entity playerEntity, LandedOnPlatformComponent& landed, Tsukino::BuiltIn::ECS::RigidbodyComponent& rb) {
            if(registry.HasComponent<PlatformComponent>(landed.platformEntity)
               && registry.HasComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(landed.platformEntity)) {
                //-------------------------------------------------------------
                // プレイヤーが土台に乗っている場合、土台の動きを止める
                //-------------------------------------------------------------
                PlatformComponent& platform                           = registry.GetComponent<PlatformComponent>(landed.platformEntity);
                platform.isMoving                                     = false;
                Tsukino::BuiltIn::ECS::RigidbodyComponent& platformRb = registry.GetComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(landed.platformEntity);
                platformRb.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Static;
                platformRb.isTypeDirty                                = true;    // PhysicsSystemにタイプ変更を通知するためのフラグを立てる
            }
            registry.RemoveComponent<LandedOnPlatformComponent>(playerEntity);
        });

        //-------------------------------------------------------------
        // viewを取得して各パドルを更新
        //-------------------------------------------------------------
        auto view = registry.View<Tsukino::BuiltIn::ECS::TransformComponent, PlatformComponent>();
        view.each([&](entt::entity entity, Tsukino::BuiltIn::ECS::TransformComponent& transform, PlatformComponent& platform) {
            if(platform.isMoving) {
                transform.position.x -= platform.speed * deltaTime;
                transform.dirty       = true;    // TransformSystemにワールド行列の更新を要求するためにフラグを立てる
            }
        });
    }
}    // namespace JumpGameSample::ECS

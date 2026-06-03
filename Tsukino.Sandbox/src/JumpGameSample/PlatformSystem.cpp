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
        auto view = registry.View<Tsukino::BuiltIn::ECS::TransformComponent, PlatformComponent, Tsukino::BuiltIn::ECS::RigidbodyComponent>();
        view.each([&](entt::entity entity, Tsukino::BuiltIn::ECS::TransformComponent& transform, PlatformComponent& platform, Tsukino::BuiltIn::ECS::RigidbodyComponent& rb) {
            if(platform.isMoving) {
                // 移動の適用
                transform.position.x -= platform.speed * deltaTime;
                transform.dirty       = true;

                //-------------------------------------------------------------
                // 0.0f を超えた（通過した）か判定
                // 右から左へ（speedが正）移動している場合：xが0.0f以下になったら停止
                // 左から右へ（speedが負）移動している場合：xが0.0f以上になったら停止
                //-------------------------------------------------------------
                bool stopCondition = (platform.speed > 0) ? (transform.position.x <= 0.0f) : (transform.position.x >= 0.0f);

                if(stopCondition) {
                    // 位置を厳密に 0.0f に固定
                    transform.position.x = 0.0f;
                    transform.dirty      = true;

                    // 停止処理
                    platform.isMoving = false;
                    rb.type           = Tsukino::BuiltIn::ECS::RigidbodyType::Static;
                    rb.isTypeDirty    = true;
                }
            }
        });
    }
}    // namespace JumpGameSample::ECS

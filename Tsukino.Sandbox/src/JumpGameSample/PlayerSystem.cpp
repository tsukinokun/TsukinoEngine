//-------------------------------------------------------------
//! @file   PlayerSystem.cpp
//! @brief  PlayerSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/JumpGameSample/ECS/System/PlayerSystem.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/PlatformComponent.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/LandedOnPlatformComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/ECS/Event/EventBus.hpp>

#include <hlsl++.h>
// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
    //-------------------------------------------------------------
    //! @brief  コンストラクタ
    //-------------------------------------------------------------
    PlayerSystem::PlayerSystem(Tsukino::ECS::EventBus& eventBus) {
        m_collisionConnection = eventBus.Subscribe<Tsukino::BuiltIn::ECS::CollisionEnterEvent>(
            [this](const Tsukino::BuiltIn::ECS::CollisionEnterEvent& e) { m_pendingCollisions.push_back(e); });
    }

    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void PlayerSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // コンテキストの取得
        //-------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx)
            return;

        Tsukino::Input::InputSystem* inputSystem = ctx->inputSystem;

        // 衝突イベントを処理
        for(auto& e : m_pendingCollisions) {
            // selfがプレイヤーでなければスキップ
             if(!registry.HasComponent<PlayerComponent>(e.self))
                continue;
            // otherが土台でなければスキップ
            if(!registry.HasComponent<PlatformComponent>(e.other))
                continue;

            auto& rb = registry.GetComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(e.self);

            if(rb.isGrounded) {
                // 上に乗った → LandedOnPlatformComponentを置く
                registry.AddComponent<LandedOnPlatformComponent>(e.self, LandedOnPlatformComponent{e.other});
            } else {
                // 横から当たった → 吹っ飛ばす
                registry.AddComponent<Tsukino::BuiltIn::ECS::ImpulseRequestComponent>(e.self, hlslpp::float3(500.0f, 300.0f, 0.0f));
            }
        }
        m_pendingCollisions.clear();

        //-------------------------------------------------------------
        // viewを取得して各パドルを更新
        //-------------------------------------------------------------
        auto view = registry.View<Tsukino::BuiltIn::ECS::TransformComponent, PlayerComponent, Tsukino::BuiltIn::ECS::RigidbodyComponent>();
        view.each([&](entt::entity                               entity,
                      Tsukino::BuiltIn::ECS::TransformComponent& transform,
                      PlayerComponent&                           player,
                      Tsukino::BuiltIn::ECS::RigidbodyComponent& rb) {
            //-------------------------------------------------------------
            // スペースキー押下でジャンプ
            //-------------------------------------------------------------
            if(rb.isGrounded && inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Space)) {
                //-------------------------------------------------------------
                // ジャンプ処理
                //-------------------------------------------------------------
                // エンティティにImpulseRequestComponentを追加して、物理システムにジャンプの衝撃を要求する
                registry.AddComponent<Tsukino::BuiltIn::ECS::ImpulseRequestComponent>(entity, hlslpp::float3(0.0f, 100.0f, 0.0f));
            }
        });
    }
}    // namespace JumpGameSample::ECS

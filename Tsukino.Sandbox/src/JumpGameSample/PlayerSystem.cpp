//-------------------------------------------------------------
//! @file   PlayerSystem.cpp
//! @brief  PlayerSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/JumpGameSample/ECS/System/PlayerSystem.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/PlayerComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>

#include <hlsl++.h>
// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
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

//-------------------------------------------------------------
//! @file   PlayerSystem.cpp
//! @brief  PlayerSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/JumpGameSample/ECS/System/PlayerSystem.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/PlatformComponent.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/LandedOnPlatformComponent.hpp>

#include <Tsukino/Sandbox/JumpGameSample/ECS/State/GameState.hpp>

#include <Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/ECS/Event/EventBus.hpp>
#include <Tsukino/Core/Path.hpp>

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
            if(!registry.HasComponent<PlayerComponent>(e.self))
                continue;
            if(!registry.HasComponent<PlatformComponent>(e.other))
                continue;

            // 衝突法線（Normal）を使う。Platformの法線が-1.0fなら真上に乗ったことになる
            bool isTopSurface = e.normal.y == -1.0f;

            if(isTopSurface) {
                // 上に乗った
                // 既存のコンポーネントがある場合は上書きするよう注意
                registry.AddComponent<LandedOnPlatformComponent>(e.self, LandedOnPlatformComponent{e.other});

                Tsukino::BuiltIn::ECS::AnimationControllerComponent& animController =
                    registry.GetComponent<Tsukino::BuiltIn::ECS::AnimationControllerComponent>(e.self);

            } else {
                // 横から当たった (または下から当たった)
                // 連続で吹っ飛ばされないように、すでにImpulseがあるかチェックするとより安全です
                if(!registry.HasComponent<Tsukino::BuiltIn::ECS::ImpulseRequestComponent>(e.self)) {
                    const hlslpp::float3 impulseStrength = hlslpp::float3(-100.0f, 0.0f, 0.0f);
                    registry.AddComponent<Tsukino::BuiltIn::ECS::ImpulseRequestComponent>(e.self, impulseStrength);
                }
            }
        }
        m_pendingCollisions.clear();

        //-------------------------------------------------------------
        // viewを取得して各パドルを更新
        //-------------------------------------------------------------
        auto view = registry.View<Tsukino::BuiltIn::ECS::TransformComponent,
                                  PlayerComponent,
                                  Tsukino::BuiltIn::ECS::RigidbodyComponent,
                                  Tsukino::BuiltIn::ECS::AnimationControllerComponent,
                                  Tsukino::BuiltIn::ECS::AnimationPlayerComponent>();
        view.each([&](entt::entity                                         entity,
                      Tsukino::BuiltIn::ECS::TransformComponent&           transform,
                      PlayerComponent&                                     player,
                      Tsukino::BuiltIn::ECS::RigidbodyComponent&           rb,
                      Tsukino::BuiltIn::ECS::AnimationControllerComponent& animController,
                      Tsukino::BuiltIn::ECS::AnimationPlayerComponent&     animPlayer) {
            //-------------------------------------------------------------
            // スペースキー押下でジャンプ
            //-------------------------------------------------------------
            if(rb.isGrounded && inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Space)) {
                JumpGameSample::ECS::GameState state = registry.GetContext<JumpGameSample::ECS::GameState>();
                if(state == JumpGameSample::ECS::GameState::Playing) {
                    //-------------------------------------------------------------
                    // ジャンプ処理
                    //-------------------------------------------------------------
                    // エンティティにImpulseRequestComponentを追加して、物理システムにジャンプの衝撃を要求する
                    const hlslpp::float3 impulseStrength = hlslpp::float3(0.0f, 100.0f, 0.0f);
                    registry.AddComponent<Tsukino::BuiltIn::ECS::ImpulseRequestComponent>(entity, impulseStrength);

                    //-------------------------------------------------------------
                    // ジャンプへ切り替え
                    //-------------------------------------------------------------
                    animController.next.clip = ctx->assetManager->Load(
                        Tsukino::Core::Path("Tsukino.Sandbox/Assets/JumpGameSample/Anims/Jumping.fbx"));    // ジャンプアニメーションに切り替える
                    animController.next.immediate = true;
                    animPlayer.is_looping         = false;    // ジャンプアニメーションはループさせない
                }
            }
        });
    }
}    // namespace JumpGameSample::ECS

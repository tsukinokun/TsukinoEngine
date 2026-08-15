//-------------------------------------------------------------
//! @file   PlayerSystem.cpp
//! @brief  PlayerSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/ActionGame/ECS/System/PlayerSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/WeaponComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/TpsCameraComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/CharacterControllerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>

#include <hlsl++.h>
#include <cmath>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
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
        // TPSカメラのyawを取得する（カメラ基準の移動方向を求めるため）
        //-------------------------------------------------------------
        float cameraYaw = 0.0f;
        {
            auto cameraView = registry.View<TpsCameraComponent>();
            if(!cameraView.empty())
                cameraYaw = cameraView.get<TpsCameraComponent>(cameraView.front()).yaw;
        }
        // yaw=0のとき-Z方向（カメラの後方）を基準とした球面座標に対応する、カメラの水平方向の前後・左右ベクトル
        hlslpp::float3 cameraForward = hlslpp::float3(-std::sin(cameraYaw), 0.0f, std::cos(cameraYaw));
        hlslpp::float3 cameraRight   = hlslpp::float3(std::cos(cameraYaw), 0.0f, std::sin(cameraYaw));

        //-------------------------------------------------------------
        // viewを取得して各プレイヤーを更新
        //-------------------------------------------------------------
        auto view = registry.View<Tsukino::BuiltIn::ECS::TransformComponent,
                                  PlayerComponent,
                                  Tsukino::BuiltIn::ECS::CharacterControllerComponent>();
        view.each([&](entt::entity                                    entity,
                      Tsukino::BuiltIn::ECS::TransformComponent&      transform,
                      PlayerComponent&                                player,
                      Tsukino::BuiltIn::ECS::CharacterControllerComponent& cc) {
            //-------------------------------------------------------------
            // 移動方向の入力を取得（カメラの向きを基準にしたXZ平面）
            //-------------------------------------------------------------
            hlslpp::float3 moveDir = hlslpp::float3(0.0f, 0.0f, 0.0f);

            if(inputSystem->IsKeyDown(Tsukino::Input::KeyCode::Up) || inputSystem->IsKeyDown(Tsukino::Input::KeyCode::W))
                moveDir = moveDir + cameraForward;
            if(inputSystem->IsKeyDown(Tsukino::Input::KeyCode::Down) || inputSystem->IsKeyDown(Tsukino::Input::KeyCode::S))
                moveDir = moveDir - cameraForward;
            if(inputSystem->IsKeyDown(Tsukino::Input::KeyCode::Right) || inputSystem->IsKeyDown(Tsukino::Input::KeyCode::D))
                moveDir = moveDir + cameraRight;
            if(inputSystem->IsKeyDown(Tsukino::Input::KeyCode::Left) || inputSystem->IsKeyDown(Tsukino::Input::KeyCode::A))
                moveDir = moveDir - cameraRight;

            float len = hlslpp::length(moveDir);
            if(len > 0.001f) {
                moveDir = moveDir / len;

                // CharacterControllerComponentへ水平方向の希望移動速度を渡す
                cc.moveInput = moveDir * player.moveSpeed;

                // 移動方向へ向き直す（瞬時に向かず、slerpで滑らかに補間する）
                float               yawRad         = std::atan2(moveDir.x, moveDir.z);
                hlslpp::quaternion targetRotation = hlslpp::quaternion::rotation_y(yawRad);
                float               turnT          = 1.0f - std::exp(-player.turnLerpSpeed * deltaTime);
                transform.rotation                 = hlslpp::slerp(transform.rotation, targetRotation, turnT);
                transform.dirty                     = true;
            } else {
                cc.moveInput = hlslpp::float3(0.0f, 0.0f, 0.0f);
            }

            //-------------------------------------------------------------
            // 接地している時のみジャンプ要求を出す
            //-------------------------------------------------------------
            if(cc.isGrounded && inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Space)) {
                cc.jumpRequested = true;
            }

            //-------------------------------------------------------------
            // 左クリックで攻撃要求を武器エンティティへ伝える
            // （実際の当たり判定の有効化・タイマー管理はCombatSystemが行う）
            //-------------------------------------------------------------
            if(inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::LButton) &&
               player.weaponEntity != entt::null &&
               registry.HasComponent<WeaponComponent>(player.weaponEntity)) {
                WeaponComponent& weapon = registry.GetComponent<WeaponComponent>(player.weaponEntity);
                weapon.attackRequested  = true;
            }
        });
    }
}    // namespace ActionGame::ECS

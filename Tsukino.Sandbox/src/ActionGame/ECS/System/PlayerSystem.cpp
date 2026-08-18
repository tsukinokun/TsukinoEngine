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
            // 攻撃中か（前フレームにPlayerAnimationSystemが確定した値）を取得。
            // 攻撃モーション中は移動方向への向き直しを止める
            //-------------------------------------------------------------
            bool isAttacking = false;
            if(player.weaponEntity != entt::null && registry.HasComponent<WeaponComponent>(player.weaponEntity)) {
                isAttacking = registry.GetComponent<WeaponComponent>(player.weaponEntity).isAttacking;
            }

            //-------------------------------------------------------------
            // isAttackingはPlayerAnimationSystem（本Systemより後に実行される）が
            // 今フレーム確定させる値なので、ここではまだ前フレームの値のまま＝
            // 攻撃ボタンを押した瞬間のフレームだけ「攻撃中ではない」と誤判定してしまい、
            // その時点の移動入力へ向き直りが1フレーム分だけ紛れ込む
            // （攻撃開始時の向き、ひいてはハンマーの振り始めの向きが移動入力に左右されてしまう
            //   不具合の原因だった）。攻撃ボタンの入力自体はこのSystem内で直接見られるので、
            // 「今フレーム攻撃がトリガーされるか」も向き直り抑制条件に含めて1フレームの遅れを潰す
            //-------------------------------------------------------------
            bool attackTriggeredThisFrame = inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::LButton);

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

                // Shift押下中はスプリント（PlayerAnimationSystemがFastRun状態の判定に使う）
                player.isSprinting  = inputSystem->IsKeyDown(Tsukino::Input::KeyCode::Shift);
                float currentSpeed = player.isSprinting ? player.moveSpeed * player.sprintSpeedMultiplier : player.moveSpeed;

                // CharacterControllerComponentへ水平方向の希望移動速度を渡す
                cc.moveInput = moveDir * currentSpeed;

                if(!isAttacking && !attackTriggeredThisFrame) {
                    // 移動方向へ向き直す（瞬時に向かず、slerpで滑らかに補間する）
                    float               yawRad         = std::atan2(moveDir.x, moveDir.z);
                    hlslpp::quaternion targetRotation = hlslpp::quaternion::rotation_y(yawRad);
                    float               turnT          = 1.0f - std::exp(-player.turnLerpSpeed * deltaTime);
                    transform.rotation                 = hlslpp::slerp(transform.rotation, targetRotation, turnT);
                    transform.dirty                     = true;
                }
            } else {
                player.isSprinting = false;
                cc.moveInput         = hlslpp::float3(0.0f, 0.0f, 0.0f);
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

            //-------------------------------------------------------------
            // マウスホイールで装備中の武器を切り替える（1ノッチ=±1.0）。
            // 攻撃中に切り替わると持ち替えの見た目が破綻するため、攻撃中は入力を無視する
            // （isAttacking/attackTriggeredThisFrameは上の向き直り抑制と同じ判定を流用）
            //-------------------------------------------------------------
            float wheelDelta = inputSystem->GetWheelDelta();
            if(!isAttacking && !attackTriggeredThisFrame &&
               wheelDelta != 0.0f && player.weaponInventory.size() > 1) {
                int direction   = wheelDelta > 0.0f ? 1 : -1;
                int weaponCount = static_cast<int>(player.weaponInventory.size());

                // 今まで選択していた武器の浮遊ブーストを解除してから、新しい武器へ差し替える
                if(registry.HasComponent<WeaponComponent>(player.weaponEntity))
                    registry.GetComponent<WeaponComponent>(player.weaponEntity).floatSelected = false;

                player.selectedWeaponIndex = (player.selectedWeaponIndex + direction + weaponCount) % weaponCount;
                player.weaponEntity         = player.weaponInventory[player.selectedWeaponIndex];

                if(registry.HasComponent<WeaponComponent>(player.weaponEntity))
                    registry.GetComponent<WeaponComponent>(player.weaponEntity).floatSelected = true;
            }
        });
    }
}    // namespace ActionGame::ECS

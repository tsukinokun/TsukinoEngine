//-------------------------------------------------------------
//! @file   PlayerMovementSystem.cpp
//! @brief  PlayerMovementSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/WaterGameSample/ECS/System/PlayerMovementSystem.hpp>

#include <Tsukino/Sandbox/WaterGameSample/ECS/Component/PlayerMovementComponent.hpp>
#include <Tsukino/Sandbox/WaterGameSample/ECS/Component/GameCameraComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/RigidBodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Input/KeyCodes.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <hlsl++.h>
#include <cmath>

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新処理
    //-------------------------------------------------------------
    void PlayerMovementSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!context || !context->inputSystem)
            return;

        auto& input = *context->inputSystem;

        //-------------------------------------------------------------
        // 矢印キー入力の取得（前後左右）
        //-------------------------------------------------------------
        float forwardInput = 0.0f;
        float rightInput   = 0.0f;

        if(input.IsKeyDown(Tsukino::Input::KeyCode::Up))
            forwardInput += 1.0f;
        if(input.IsKeyDown(Tsukino::Input::KeyCode::Down))
            forwardInput -= 1.0f;
        if(input.IsKeyDown(Tsukino::Input::KeyCode::Right))
            rightInput += 1.0f;
        if(input.IsKeyDown(Tsukino::Input::KeyCode::Left))
            rightInput -= 1.0f;

        auto view = registry.View<PlayerMovementComponent, Tsukino::BuiltIn::ECS::RigidbodyComponent>();

        view.each([&](auto entity, auto& move, auto& rb) {
            //-------------------------------------------------------------
            // 入力が無い／カメラ未設定なら力を0クリアして終了
            //    （AddForceは消費されないので、離した瞬間に明示的に0にする必要がある）
            //-------------------------------------------------------------
            bool noInput  = (forwardInput == 0.0f && rightInput == 0.0f);
            bool noCamera = (move.cameraEntity == entt::null || !registry.HasComponent<GameCameraComponent>(move.cameraEntity));

            if(noInput || noCamera) {
                rb.force  = {0, 0, 0};
                rb.torque = {0, 0, 0};
                return;
            }

            auto& gameCam = registry.GetComponent<GameCameraComponent>(move.cameraEntity);
            float yaw     = gameCam.yaw;

            //-------------------------------------------------------------
            // カメラのyawから「ボールの前方向」「右方向」を計算
            //-------------------------------------------------------------
            hlslpp::float3 forward(-std::sin(yaw), 0.0f, std::cos(yaw));
            hlslpp::float3 right(std::cos(yaw), 0.0f, std::sin(yaw));

            hlslpp::float3 moveDir = forward * forwardInput + right * rightInput;

            float lenSq = moveDir.x * moveDir.x + moveDir.z * moveDir.z;
            if(lenSq > 0.0001f) {
                float invLen  = 1.0f / std::sqrt(lenSq);
                moveDir.x    *= invLen;
                moveDir.z    *= invLen;
            }

            //-------------------------------------------------------------
            // 力（毎フレーム加え続ける。deltaTimeは掛けない = F = m * a の"力"そのもの）
            //-------------------------------------------------------------
            rb.force = moveDir * move.moveForce;
        });
    }

}    // namespace WaterGame::ECS

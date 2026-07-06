//-------------------------------------------------------------
//! @file   GameCameraSystem.cpp
//! @brief  GameCameraSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/WaterGameSample/ECS/System/GameCameraSystem.hpp>

#include <Tsukino/Sandbox/WaterGameSample/ECS/Component/GameCameraComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Input/KeyCodes.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <hlsl++.h>

#include <algorithm>

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    //-------------------------------------------------------------
    // システムの更新処理
    //-------------------------------------------------------------
    void GameCameraSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!context || !context->inputSystem)
            return;

        auto& input = *context->inputSystem;

        //-------------------------------------------------------------
        // WASD入力によるyaw / pitchの入力値を先に計算（全エンティティ共通）
        //    A/D : 水平回転（yaw）
        //    W/S : 上下回転（pitch）
        //-------------------------------------------------------------
        float yawInput   = 0.0f;
        float pitchInput = 0.0f;

        if(input.IsKeyDown(Tsukino::Input::KeyCode::A))
            yawInput -= 1.0f;
        if(input.IsKeyDown(Tsukino::Input::KeyCode::D))
            yawInput += 1.0f;
        if(input.IsKeyDown(Tsukino::Input::KeyCode::W))
            pitchInput += 1.0f;
        if(input.IsKeyDown(Tsukino::Input::KeyCode::S))
            pitchInput -= 1.0f;


        auto view = registry.View<GameCameraComponent, Tsukino::BuiltIn::ECS::TransformComponent, Tsukino::BuiltIn::ECS::CameraComponent>();

        // eachを回す
        view.each([&](auto entity, auto& gameCam, auto& transform, auto& camera) {
            if(gameCam.target == entt::null || !registry.HasComponent<Tsukino::BuiltIn::ECS::TransformComponent>(gameCam.target))
                return;

            //-------------------------------------------------------------
            // yaw / pitchの更新
            //-------------------------------------------------------------
            gameCam.yaw   += yawInput * gameCam.rotateSpeed * deltaTime;
            gameCam.pitch += pitchInput * gameCam.rotateSpeed * deltaTime;
            gameCam.pitch  = std::clamp(gameCam.pitch, gameCam.minPitch, gameCam.maxPitch);

            //-------------------------------------------------------------
            // 球面座標からターゲット基準のオフセットを計算
            //    yaw=0, pitch=0 のとき Z-負方向（後方）を基準とする
            //-------------------------------------------------------------
            float horizontalDist = gameCam.distance * std::cos(gameCam.pitch);

            hlslpp::float3 offset;
            offset.x = horizontalDist * std::sin(gameCam.yaw);
            offset.y = gameCam.height + gameCam.distance * std::sin(gameCam.pitch);
            offset.z = -horizontalDist * std::cos(gameCam.yaw);

            auto&          targetTransform = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(gameCam.target);
            hlslpp::float3 desiredPos      = targetTransform.position + offset;

            //-------------------------------------------------------------
            // 位置の補間追従（急な追従にならないようlerp）
            //-------------------------------------------------------------
            float t            = 1.0f - std::exp(-gameCam.followLerpSpeed * deltaTime);
            transform.position = transform.position + (desiredPos - transform.position) * t;
            transform.dirty    = true;

            //-------------------------------------------------------------
            // カメラの注視点をターゲットに設定
            //-------------------------------------------------------------
            camera.lookAtTarget = targetTransform.position;
        });
    }

}    // namespace WaterGame::ECS

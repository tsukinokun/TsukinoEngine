//-------------------------------------------------------------
//! @file   DebugCameraSystem.cpp
//! @brief  デバッグカメラシステムの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#ifdef _DEBUG

#include <Tsukino/EngineIntegration/ECS/System/DebugCameraSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraTag.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Input/KeyCodes.hpp>
#include <Tsukino/Core/Math/MathHelper.hpp>

#include <hlsl++.h>
#include <algorithm>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @brief  デバッグカメラの操作を管理するシステムの更新関数
    //-------------------------------------------------------------
    void DebugCameraSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->inputSystem)
            return;

        auto& input = *ctx->inputSystem;

        auto view = registry.View<TransformComponent, CameraComponent, DebugCameraComponent>();
        view.each([&](entt::entity entity, TransformComponent& transform, CameraComponent& camera, DebugCameraComponent& cam) {
            //----------------------------------------------------------
            // F5 でデバッグカメラのオン/オフをトグル
            //----------------------------------------------------------
            if(input.IsKeyPressed(Tsukino::Input::KeyCode::F5)) {
                cam.isActive = !cam.isActive;

                auto camView = registry.View<CameraComponent>();
                camView.each([&](entt::entity e, CameraComponent& c) {
                    if(registry.HasComponent<DebugCameraTag>(e))
                        return;

                    // オフにするときは必ず3Dカメラ（Perspective）に戻す
                    if(!cam.isActive) {
                        c.isPrimary = (c.projectionType == CameraComponent::ProjectionType::Perspective);
                    } else {
                        c.isPrimary = false;
                    }
                    c.dirty = true;
                });

                camera.isPrimary = cam.isActive;
                camera.dirty     = true;
            }

            if(!cam.isActive)
                return;

            //----------------------------------------------------------
            // 右クリック押しっぱなしの間だけ全操作を受け付ける（Unreal準拠）
            //----------------------------------------------------------
            if(!input.IsKeyDown(Tsukino::Input::KeyCode::RButton))
                return;

            //----------------------------------------------------------
            // マウスで視点回転
            //----------------------------------------------------------
            i32 dx = 0, dy = 0;
            input.GetMouseDelta(&dx, &dy);

            cam.yaw   += dx * cam.mouseSens;
            cam.pitch += dy * cam.mouseSens;
            cam.pitch  = std::clamp(cam.pitch, -89.0f, 89.0f);

            float yawRad   = Tsukino::Core::Math::ToRadians(cam.yaw);
            float pitchRad = Tsukino::Core::Math::ToRadians(cam.pitch);

            hlslpp::quaternion qYaw   = hlslpp::quaternion::rotation_y(yawRad);
            hlslpp::quaternion qPitch = hlslpp::quaternion::rotation_x(pitchRad);
            transform.rotation        = hlslpp::mul(qYaw, qPitch);

            //----------------------------------------------------------
            // WASD + EQ で移動（右クリック中のみ）
            //----------------------------------------------------------
            float speed = input.IsKeyDown(Tsukino::Input::KeyCode::Shift) ? cam.sprintSpeed : cam.moveSpeed;

            hlslpp::float3 forward = hlslpp::mul(transform.rotation, hlslpp::float3(0, 0, 1));
            hlslpp::float3 right   = hlslpp::mul(transform.rotation, hlslpp::float3(1, 0, 0));
            hlslpp::float3 up      = hlslpp::float3(0, 1, 0);

            hlslpp::float3 move = hlslpp::float3(0, 0, 0);

            if(input.IsKeyDown(Tsukino::Input::KeyCode::W))
                move = move + forward;
            if(input.IsKeyDown(Tsukino::Input::KeyCode::S))
                move = move - forward;
            if(input.IsKeyDown(Tsukino::Input::KeyCode::D))
                move = move + right;
            if(input.IsKeyDown(Tsukino::Input::KeyCode::A))
                move = move - right;
            if(input.IsKeyDown(Tsukino::Input::KeyCode::E))
                move = move + up;
            if(input.IsKeyDown(Tsukino::Input::KeyCode::Q))
                move = move - up;

            float len = hlslpp::length(move);
            if(len > 0.001f) {
                transform.position = transform.position + (move / len) * speed;
            }

            //----------------------------------------------------------
            // スクロールで移動速度を調整
            //----------------------------------------------------------
            cam.moveSpeed += input.GetWheelDelta() * 0.5f;
            cam.moveSpeed  = std::clamp(cam.moveSpeed, 1.0f, 100.0f);

            transform.dirty = true;
        });
    }

}    // namespace Tsukino::BuiltIn::ECS

#endif    // _DEBUG

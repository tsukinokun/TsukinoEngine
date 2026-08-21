//-------------------------------------------------------------
//! @file   TpsCameraSystem.cpp
//! @brief  TpsCameraSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/ActionGame/ECS/System/TpsCameraSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/TpsCameraComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Window.hpp>

#include <hlsl++.h>
#include <cmath>
#include <algorithm>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void TpsCameraSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx)
            return;

        Tsukino::Input::InputSystem* inputSystem = ctx->inputSystem;

        //-------------------------------------------------------------
        // マウスの移動量を取得（このフレームの旋回入力）
        //-------------------------------------------------------------
        i32 rawMouseDx = 0, rawMouseDy = 0;
        inputSystem->GetMouseDelta(&rawMouseDx, &rawMouseDy);

        //-------------------------------------------------------------
        // ウィンドウがフォーカスされているか（Alt+Tab等で裏にいる間はカーソルを奪わない）
        //-------------------------------------------------------------
        bool windowFocused = ctx->window && ctx->window->IsFocused();

        auto view = registry.View<TpsCameraComponent, Tsukino::BuiltIn::ECS::TransformComponent, Tsukino::BuiltIn::ECS::CameraComponent>();
        view.each([&](entt::entity                                  entity,
                     TpsCameraComponent&                            tpsCamera,
                     Tsukino::BuiltIn::ECS::TransformComponent&    transform,
                     Tsukino::BuiltIn::ECS::CameraComponent&       camera) {
            //-------------------------------------------------------------
            // 追従対象が未設定なら、プレイヤーエンティティを探して設定する
            //-------------------------------------------------------------
            if(tpsCamera.target == entt::null) {
                auto playerView = registry.View<PlayerComponent>();
                if(!playerView.empty())
                    tpsCamera.target = playerView.front();
            }

            if(tpsCamera.target == entt::null || !registry.HasComponent<Tsukino::BuiltIn::ECS::TransformComponent>(tpsCamera.target))
                return;

            Tsukino::BuiltIn::ECS::TransformComponent& targetTransform =
                registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(tpsCamera.target);

            //-------------------------------------------------------------
            // Escキーでマウスキャプチャ（カーソル非表示＋旋回操作）のON/OFFを切り替える
            //-------------------------------------------------------------
            if(windowFocused && inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Escape)) {
                tpsCamera.mouseCaptured = !tpsCamera.mouseCaptured;
            }

            bool shouldCapture = tpsCamera.mouseCaptured && windowFocused;

            if(ctx->window)
                ctx->window->SetCursorVisible(!shouldCapture);

            i32 mouseDx = rawMouseDx;
            i32 mouseDy = rawMouseDy;

            if(shouldCapture) {
                // 直前フレームまでキャプチャが外れていた場合、カーソルがどこにあったか分からないため
                // このフレームの分は旋回に使わず、中央への位置合わせだけ行う
                if(!tpsCamera.wasCapturedLastFrame) {
                    mouseDx = 0;
                    mouseDy = 0;
                }

                //-------------------------------------------------------------
                // マウス移動量でyaw/pitchを更新する
                //-------------------------------------------------------------
                tpsCamera.yaw += static_cast<float>(mouseDx) * tpsCamera.mouseSensitivity;
                tpsCamera.pitch += static_cast<float>(mouseDy) * tpsCamera.mouseSensitivity;
                tpsCamera.pitch = std::clamp(tpsCamera.pitch, tpsCamera.minPitch, tpsCamera.maxPitch);

                //-------------------------------------------------------------
                // カーソルを中央へ戻し、InputSystem側の座標も同期させる
                // （同期しないと、戻した分が次フレームの移動量として誤検出される）
                //-------------------------------------------------------------
                if(ctx->window) {
                    ctx->window->CenterCursor();

                    i32 centerX = ctx->window->GetWidth() / 2;
                    i32 centerY = ctx->window->GetHeight() / 2;
                    inputSystem->SetMousePosition(centerX, centerY);
                }
            }

            tpsCamera.wasCapturedLastFrame = shouldCapture;

            //-------------------------------------------------------------
            // yaw=0, pitch=0のとき-Z方向（後方）を基準とした球面座標でオフセットを求める
            //-------------------------------------------------------------
            float horizontalDist = tpsCamera.distance * std::cos(tpsCamera.pitch);

            hlslpp::float3 offset;
            offset.x = horizontalDist * std::sin(tpsCamera.yaw);
            offset.y = tpsCamera.height + tpsCamera.distance * std::sin(tpsCamera.pitch);
            offset.z = -horizontalDist * std::cos(tpsCamera.yaw);

            hlslpp::float3 desiredPosition = targetTransform.position + offset;

            //-------------------------------------------------------------
            // 急な追従にならないよう指数減衰で補間する
            //-------------------------------------------------------------
            float t             = 1.0f - std::exp(-tpsCamera.followLerpSpeed * deltaTime);
            transform.position  = transform.position + (desiredPosition - transform.position) * t;
            transform.dirty     = true;

            //-------------------------------------------------------------
            // プレイヤーの頭のあたりを注視する
            //-------------------------------------------------------------
            camera.useLookAt    = true;
            camera.lookAtTarget = targetTransform.position + hlslpp::float3(0.0f, tpsCamera.lookHeight, 0.0f);
            camera.dirty        = true;
        });
    }
}    // namespace ActionGame::ECS

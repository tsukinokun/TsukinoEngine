//-------------------------------------------------------------
//! @file   CameraSystem.cpp
//! @brief  CameraSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UICanvas.hpp>

#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <Tsukino/Core/Window.hpp>
#include <Tsukino/Core/Math/MathHelper.hpp>

#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void CameraSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // コンテキストの取得
        //-------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

        //-------------------------------------------------------------
        // アスペクト比を算出
        //-------------------------------------------------------------
        float screenW       = static_cast<float>(ctx->window->GetWidth());
        float screenH       = static_cast<float>(ctx->window->GetHeight());
        float currentAspect = screenW / screenH;

        //-------------------------------------------------------------
        // 画面の大きさが変わったら、全カメラの行列を作り直す。
        // 縦横比だけを見ていると、同じ縦横比のまま広げたとき（1280x720 → 1920x1080）に
        // UI カメラが作り直されず、UI が引き伸ばされる
        //-------------------------------------------------------------
        const bool screenResized = (screenW != m_lastScreenWidth || screenH != m_lastScreenHeight);
        m_lastScreenWidth        = screenW;
        m_lastScreenHeight       = screenH;

        // UI の座標と画面のピクセルの対応（UI カメラが見つからなければ UI の座標 ＝ ピクセル）
        UICanvas canvas = UICanvas::Fit(hlslpp::float2(0.0f, 0.0f), hlslpp::float2(screenW, screenH));

        //-------------------------------------------------------------
        // viewを取得して各カメラエンティティを更新
        //-------------------------------------------------------------
        auto view = registry.View<TransformComponent, CameraComponent>();
        view.each([&](entt::entity entity, const Tsukino::BuiltIn::ECS::TransformComponent& transform, Tsukino::BuiltIn::ECS::CameraComponent& camera) {
            // アスペクト比が前回計算時と異なれば Dirty フラグを立てる
            if(camera.aspectRatio != currentAspect || screenResized) {
                camera.aspectRatio = currentAspect;
                camera.dirty       = true;
            }

            // メインでない正射影のカメラは画面 UI のカメラ。基準の解像度から UI の座標と画面の対応を決める
            const bool isUICamera = !camera.isPrimary && camera.projectionType == CameraComponent::ProjectionType::Orthographic;
            if(isUICamera)
                canvas = UICanvas::Fit(camera.referenceResolution, hlslpp::float2(screenW, screenH));

            //-------------------------------------------------------------
            // Transform か Camera のパラメータが変わっていれば行列を更新
            //-------------------------------------------------------------
            if(transform.dirty || camera.dirty) {
                //-------------------------------------------------------------
                // View行列の計算 (カメラの向き)
                //-------------------------------------------------------------
                // 上方ベクトルは常にRotationから算出（または(0,1,0)固定でも可）
                hlslpp::float3 up = hlslpp::mul(transform.rotation, hlslpp::float3(0, 1, 0));
                hlslpp::float3 target;

                //-------------------------------------------------------------
                // 注視を使用する場合の処理
                //-------------------------------------------------------------
                if(camera.useLookAt) {
                    target = camera.lookAtTarget;
                } else {
                    hlslpp::float3 forward = hlslpp::mul(transform.rotation, hlslpp::float3(0, 0, 1));
                    target                 = transform.position + forward;
                }

                // 決定した target を使って View行列を作成
                camera.viewMatrix = Tsukino::Core::Math::matrix::lookAtLH(transform.position, target, up);

                //-------------------------------------------------------------
                // Projection行列の計算 (投影方法の分岐)
                //-------------------------------------------------------------
                if(camera.projectionType == CameraComponent::ProjectionType::Orthographic) {
                    // 画面に映る UI の座標の範囲（基準が無ければ 0〜screenW, 0〜screenH）。
                    // Bottom に上端、Top に下端を渡して上下反転する（Sprite.vs.hlsl が Y を反転するので左上原点になる）
                    const UICanvas       fit      = isUICamera ? canvas : UICanvas::Fit(camera.referenceResolution, hlslpp::float2(screenW, screenH));
                    const hlslpp::float2 topLeft  = fit.ToUI(hlslpp::float2(0.0f, 0.0f));
                    const hlslpp::float2 lowRight = fit.ToUI(hlslpp::float2(screenW, screenH));
                    camera.projectionMatrix       = Tsukino::Core::Math::matrix::orthographicOffCenterLH(float(topLeft.x), float(lowRight.x),    // Left, Right
                                                                                                         float(topLeft.y), float(lowRight.y),    // Bottom, Top
                                                                                                         camera.nearZ, camera.farZ);

                } else {
                    // 自作の perspectiveFovLH を使用
                    // リバースZに対応しているため、farZ と nearZ の順番で指定
                    camera.projectionMatrix = Tsukino::Core::Math::matrix::perspectiveFovLH(
                        Tsukino::Core::Math::ToRadians(camera.fov), camera.aspectRatio, camera.farZ, camera.nearZ);
                }

                //-------------------------------------------------------------
                // ViewProjection行列の合成
                //-------------------------------------------------------------
                camera.viewProjMatrix = hlslpp::mul(camera.viewMatrix, camera.projectionMatrix);

                //-------------------------------------------------------------
                //! @brief ViewProjection行列の逆行列の計算
                //-------------------------------------------------------------
                camera.invViewProjMatrix = hlslpp::inverse(camera.viewProjMatrix);
                // 更新完了
                camera.dirty = false;
            }
        });

        // UI を扱うシステム（文字・切り取り・マウスの当たり判定など）が使う、UI の座標と画面のピクセルの対応
        registry.SetContext<UICanvas>() = canvas;

        //-------------------------------------------------------------
        // viewを再度ループして、シーン定数バッファを更新
        //-------------------------------------------------------------
        view.each([&](entt::entity entity, const Tsukino::BuiltIn::ECS::TransformComponent& transform, const Tsukino::BuiltIn::ECS::CameraComponent& camera) {
            // メインカメラの行列をシーン定数バッファに転送
            Tsukino::Renderer::CBufferScene sceneData;
            sceneData.view        = camera.viewMatrix;
            sceneData.projection  = camera.projectionMatrix;
            sceneData.viewProj    = camera.viewProjMatrix;
            sceneData.invViewProj = camera.invViewProjMatrix;
            sceneData.cameraPos   = hlslpp::float4(transform.position.x, transform.position.y, transform.position.z, 1.0f);
            // シーン定数バッファをRendererにセット
            if(camera.isPrimary) {
                ctx->renderer->GetFrameConstants().SetWorldCamera(sceneData);
            } else {
                ctx->renderer->GetFrameConstants().SetOverlayCamera(sceneData);
            }
        });
    }

}    // namespace Tsukino::BuiltIn::ECS

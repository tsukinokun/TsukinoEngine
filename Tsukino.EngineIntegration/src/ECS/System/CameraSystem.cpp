//-------------------------------------------------------------
//! @file   CameraSystem.cpp
//! @brief  CameraSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>

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
        // viewを取得して各カメラエンティティを更新
        //-------------------------------------------------------------
        auto view = registry.View<TransformComponent, CameraComponent>();
        view.each([&](entt::entity entity, const Tsukino::BuiltIn::ECS::TransformComponent& transform, Tsukino::BuiltIn::ECS::CameraComponent& camera) {
            // アスペクト比が前回計算時と異なれば Dirty フラグを立てる
            if(camera.aspectRatio != currentAspect) {
                camera.aspectRatio = currentAspect;
                camera.dirty       = true;
            }

            //-------------------------------------------------------------
            // Transform か Camera のパラメータが変わっていれば行列を更新
            //-------------------------------------------------------------
            if(transform.dirty || camera.dirty) {
                //-------------------------------------------------------------
                // View行列の計算 (カメラの向き)
                //-------------------------------------------------------------
                // クォータニオンから前方・上方を算出
                hlslpp::float3 forward = hlslpp::mul(transform.rotation, hlslpp::float3(0, 0, 1));
                hlslpp::float3 up      = hlslpp::mul(transform.rotation, hlslpp::float3(0, 1, 0));
                hlslpp::float3 target  = transform.position + forward;

                camera.viewMatrix = Tsukino::Core::Math::matrix::lookAtLH(transform.position, target, up);

                //-------------------------------------------------------------
                // Projection行列の計算 (投影方法の分岐)
                //-------------------------------------------------------------
                if(camera.projectionType == CameraComponent::ProjectionType::Orthographic) {
                    // --- 正投影 (2D / UI用) ---
                    float halfH = camera.orthoSize * 0.5f;
                    float halfW = (camera.orthoSize * camera.aspectRatio) * 0.5f;

                    // 自作のorthographicOffCenterLH を使用
                    // Left, Right, Bottom, Top の順に指定
                    camera.projectionMatrix = Tsukino::Core::Math::matrix::orthographicOffCenterLH(-halfW,
                                                                                                   halfW,    // Left, Right
                                                                                                   -halfH,
                                                                                                   halfH,    // Bottom, Top
                                                                                                   camera.nearZ,
                                                                                                   camera.farZ);
                } else {
                    // 自作の perspectiveFovLH を使用
                    camera.projectionMatrix = Tsukino::Core::Math::matrix::perspectiveFovLH(
                        Tsukino::Core::Math::ToRadians(camera.fov), camera.aspectRatio, camera.nearZ, camera.farZ);
                }

                //-------------------------------------------------------------
                // ViewProjection行列の合成
                //-------------------------------------------------------------
                camera.viewProjMatrix = hlslpp::mul(camera.viewMatrix, camera.projectionMatrix);

                // 更新完了
                camera.dirty = false;
            }
        });
    }

}    // namespace Tsukino::BuiltIn::ECS

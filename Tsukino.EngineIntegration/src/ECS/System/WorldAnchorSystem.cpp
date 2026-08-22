//-------------------------------------------------------------
//! @file   WorldAnchorSystem.cpp
//! @brief  WorldAnchorSystemクラスの実装
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/WorldAnchorSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/WorldAnchorComponent.hpp>

#include <Tsukino/Core/Window.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>

#include <hlsl++.h>
#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void WorldAnchorSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // コンテキストの取得
        //-------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->window)
            return;

        //-------------------------------------------------------------
        // メインカメラ（isPrimary=true）のViewProjectionを取得。
        // ワールド→スクリーン変換に使う
        //-------------------------------------------------------------
        Tsukino::Core::Math::matrix cameraViewProj;
        bool                        hasCamera = false;
        {
            auto cameraView = registry.View<CameraComponent>();
            for(auto entity : cameraView) {
                const auto& camera = cameraView.get<CameraComponent>(entity);
                if(camera.isPrimary) {
                    cameraViewProj = camera.viewProjMatrix;
                    hasCamera      = true;
                    break;
                }
            }
        }

        float screenWidth  = static_cast<float>(ctx->window->GetWidth());
        float screenHeight = static_cast<float>(ctx->window->GetHeight());

        //-------------------------------------------------------------
        // WorldAnchorComponentを持つ各エンティティについて、targetのワールド座標を
        // スクリーン座標へ投影し、自分自身のTransformComponent.positionへ書き込む
        //-------------------------------------------------------------
        auto view = registry.View<WorldAnchorComponent, TransformComponent>();
        view.each([&](entt::entity, WorldAnchorComponent& anchor, TransformComponent& transform) {
            if(!hasCamera) {
                anchor.visible = false;
                return;
            }

            //-------------------------------------------------------------
            // 投影の基準となるワールド座標を決める。
            // useFixedWorldPositionのときはtargetを一切参照しないため、
            // 追従対象が破棄された後も正しい位置に貼り付いたままになる
            //-------------------------------------------------------------
            hlslpp::float3 basePosition;
            if(anchor.useFixedWorldPosition) {
                basePosition = anchor.fixedWorldPosition;
            } else {
                if(anchor.target == entt::null || !registry.IsValid(anchor.target) || !registry.HasComponent<TransformComponent>(anchor.target)) {
                    anchor.visible = false;
                    return;
                }
                basePosition = registry.GetComponent<TransformComponent>(anchor.target).position;
            }

            // 対象のワールド座標をクリップ空間へ（hlslppは行ベクトル規約なのでmul(v, m)の順）
            hlslpp::float3 worldPos = basePosition + anchor.worldOffset;
            hlslpp::float4 clip     = hlslpp::mul(hlslpp::float4(worldPos, 1.0f), cameraViewProj);

            if(clip.w <= 0.0f) {    // カメラ後方は表示しない
                anchor.visible = false;
                return;
            }

            hlslpp::float2 ndc = hlslpp::float2(clip.x, clip.y) / clip.w;
            // FontRendererSystem/SpriteRendererSystemはビューポート左上原点のピクセル座標を期待する
            float screenX = (ndc.x * 0.5f + 0.5f) * screenWidth;
            float screenY = (0.5f - ndc.y * 0.5f) * screenHeight;

            transform.position = hlslpp::float3(screenX + anchor.screenOffset.x, screenY + anchor.screenOffset.y, 0.0f);
            transform.dirty    = true;
            anchor.visible      = true;
        });
    }
}    // namespace Tsukino::BuiltIn::ECS

//-------------------------------------------------------------
//! @file   InteractionSystem.cpp
//! @brief  InteractionSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/InteractionSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DraggableComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/Math/MathHelper.hpp>

#include <hlsl++.h>
#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    namespace {
        //-------------------------------------------------------------
        //! @brief  スプライトの実サイズ（スケール考慮）を求め、指定座標が
        //!         その矩形内にあるかを判定する
        //! @return テクスチャアセットが取得できた場合は true
        //-------------------------------------------------------------
        bool ComputeSpriteHit(Tsukino::EngineIntegration::EngineContext* ctx, const TransformComponent& transform,
                               const SpriteComponent& sprite, const hlslpp::float2& point, bool* outInside) {
            auto textureAsset = std::static_pointer_cast<Tsukino::Asset::TextureAsset>(ctx->assetManager->Get(sprite.textureHandle));
            if(!textureAsset)
                return false;

            // スケールを考慮した実際の描画サイズを計算
            hlslpp::float2 spriteSize = {static_cast<float>(textureAsset->width) * transform.scale.x,
                                         static_cast<float>(textureAsset->height) * transform.scale.y};
            hlslpp::float2 spritePos = {transform.position.x, transform.position.y};

            *outInside = Tsukino::Core::Math::IsPointInRect(point, spritePos, spriteSize);
            return true;
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void InteractionSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

        auto& input = ctx->inputSystem;
        i32   mouseX, mouseY;
        input->GetMousePosition(&mouseX, &mouseY);
        hlslpp::float2 mousePos = {(float)mouseX, (float)mouseY};

        registry.View<TransformComponent, DraggableComponent, SpriteComponent>().each([&](auto entity, auto& transform, auto& draggable, auto& sprite) {
            //-------------------------------------------------------------
            // 当たり判定
            //-------------------------------------------------------------
            bool isInside = false;
            if(!ComputeSpriteHit(ctx, transform, sprite, mousePos, &isInside))
                return;

            // 以下、ドラッグ処理
            if(!draggable.isDragging && input->IsKeyPressed(Input::KeyCode::LButton) && isInside) {
            draggable.isDragging = true;
            draggable.dragOffset = mousePos - hlslpp::float2(transform.position.x, transform.position.y);
            }

            if(draggable.isDragging) {
            if(input->IsKeyDown(Input::KeyCode::LButton)) {
                transform.position.x = mousePos.x - draggable.dragOffset.x;
                transform.position.y = mousePos.y - draggable.dragOffset.y;
            } else {
                draggable.isDragging = false;
            }
            }
    });
}

    //-------------------------------------------------------------
    //! @brief  指定座標がドラッグ可能なスプライト上、またはドラッグ中の
    //!         スプライトが存在するかを調べる関数
    //-------------------------------------------------------------
    bool InteractionSystem::HitTest(Tsukino::ECS::Registry& registry, float x, float y) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx)
            return false;

        hlslpp::float2 point = {x, y};
        bool           hit   = false;

        registry.View<TransformComponent, DraggableComponent, SpriteComponent>().each([&](auto entity, auto& transform, auto& draggable, auto& sprite) {
            if(hit)
                return;

            // ドラッグ中はカーソルがドラッグオフセット分ずれて矩形外に出ることが
            // あるため、そのままクリック透過扱いになるのを防ぐ
            if(draggable.isDragging) {
                hit = true;
                return;
            }

            bool isInside = false;
            if(ComputeSpriteHit(ctx, transform, sprite, point, &isInside) && isInside) {
                hit = true;
            }
        });

        return hit;
    }

}    // namespace Tsukino::BuiltIn::ECS

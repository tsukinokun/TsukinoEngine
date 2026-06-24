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

#include <hlsl++.h>
#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
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
            // テクスチャアセットを取得してサイズを取得
            auto textureAsset = std::static_pointer_cast<Tsukino::Asset::TextureAsset>(ctx->assetManager->Get(sprite.textureHandle));
            if(!textureAsset)
                return;

            // スケールを考慮した実際の描画サイズを計算
            float w = static_cast<float>(textureAsset->width) * transform.scale.x;
            float h = static_cast<float>(textureAsset->height) * transform.scale.y;

            // 3. 当たり判定（Transformの位置を左上と仮定）
            bool isInside = (mousePos.x >= transform.position.x && mousePos.x <= transform.position.x + w && mousePos.y >= transform.position.y
                             && mousePos.y <= transform.position.y + h);

            // 以下、ドラッグ処理（変更なし）
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

}    // namespace Tsukino::BuiltIn::ECS

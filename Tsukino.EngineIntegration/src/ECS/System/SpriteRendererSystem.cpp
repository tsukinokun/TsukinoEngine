//-------------------------------------------------------------
//! @file   SpriteRendererSystem.cpp
//! @brief  SpriteRenderSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>

#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/DrawCommand.hpp>

#include <hlsl++.h>
#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void SpriteRenderSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        // TransformComponent と SpriteComponent の両方を持つエンティティを取得
        auto view = registry.View<TransformComponent, SpriteComponent>();

        // 各エンティティから情報を抽出して描画コマンドを作成する
        view.each([&](auto entity, const auto& transform, const auto& sprite) {
            Tsukino::Renderer::DrawCommand cmd;

            // Tsukino::Core::Math::matrix を渡す
            cmd.transform = transform.worldMatrix;



            // cmd.material = ...; // sprite.textureHandleや色情報からマテリアルを構築/取得
            // cmd.mesh = ...;     // Quadメッシュの取得

            // 取得したrendererに描画コマンドを積む
            // if (renderer) renderer->PushDrawCommand(cmd);
        });
    }
}    // namespace Tsukino::BuiltIn::ECS

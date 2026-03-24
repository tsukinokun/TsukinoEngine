//-------------------------------------------------------------
//! @file   SpriteRendererSystem.cpp
//! @brief  SpriteRenderSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>

#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/DrawCommand.hpp>
#include <Tsukino/Renderer/DX11/Material.hpp>

#include <Tsukino/GraphicsCommon/Mesh/PrimitiveType.hpp>

#include <hlsl++.h>
#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void SpriteRenderSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        // RegistryにセットされたEngineContextのポインタを取得
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->renderer)
            return;

        // TransformComponent と SpriteComponent の両方を持つエンティティを取得
        auto view = registry.View<TransformComponent, SpriteComponent>();

        // 各エンティティから情報を抽出して描画コマンドを作成する
        view.each([&](auto entity, const auto& transform, const auto& sprite) {
            Tsukino::Renderer::DrawCommand cmd;

            //-------------------------------------------------------------
            // Tsukino::Core::Math::matrix を渡す
            //-------------------------------------------------------------
            cmd.transform = transform.worldMatrix;

            //-------------------------------------------------------------
            // メッシュの指定
            //-------------------------------------------------------------
            cmd.mesh = ctx->renderer->GetPrimitiveMesh(Tsukino::GraphicsCommon::PrimitiveType::Quad);

            //-------------------------------------------------------------
            // マテリアルの構築
            //-------------------------------------------------------------
            //-------------------------------------------------------------
            // サンプラー設定
            //-------------------------------------------------------------
            cmd.material->SetSampler(ctx->renderer->GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp));    // リニアフィルタを指定

            // コンテキストから取得したレンダラーを使って描画
            ctx->renderer->PushDrawCommand(cmd);
        });
    }
}    // namespace Tsukino::BuiltIn::ECS

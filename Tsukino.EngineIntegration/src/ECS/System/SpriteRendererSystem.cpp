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
#include <Tsukino/Renderer/DX11/Texture/DX11Texture2D.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

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
        view.each([&](entt::entity, const Tsukino::BuiltIn::ECS::TransformComponent& transform, const Tsukino::BuiltIn::ECS::SpriteComponent& sprite) {
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
            // マテリアルの実体をローカル変数として作成
            Tsukino::Renderer::Material material;
            //-------------------------------------------------------------
            // サンプラー設定
            //-------------------------------------------------------------
            material.SetSampler(ctx->renderer->GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp));    // リニアフィルタを指定

            // テクスチャ設定
            Tsukino::Core::Ref<Tsukino::Asset::IAsset> asset = ctx->assets->Get(sprite.textureHandle);
            if(asset && asset->GetType() == Tsukino::Asset::AssetType::Texture) {
                Tsukino::Core::Ref<Tsukino::Asset::TextureAsset> textureAsset = std::static_pointer_cast<Tsukino::Asset::TextureAsset>(asset);
                ID3D11ShaderResourceView*                        srv          = ctx->renderer->GetTextureSRV(*textureAsset);
                material.SetTexture(srv);
            } else {
                material.SetTexture(nullptr);
            }

            // 実体のアドレスを DrawCommand に渡す
            cmd.material = &material;

            // コンテキストから取得したレンダラーを使って描画
            ctx->renderer->PushDrawCommand(cmd);
        });
    }
}    // namespace Tsukino::BuiltIn::ECS

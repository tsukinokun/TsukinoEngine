//-------------------------------------------------------------
//! @file   SpriteRendererSystem.cpp
//! @brief  SpriteRenderSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/BuiltInAssets.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>

#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/DrawCommand.hpp>
#include <Tsukino/Renderer/DX11/Material.hpp>
#include <Tsukino/Renderer/DX11/Texture/DX11Texture2D.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

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

        // 初回のみパイプラインを取得・生成する
        if(!m_pipelineCache) {
            //-------------------------------------------------------------
            // VS
            //-------------------------------------------------------------
            std::shared_ptr<Tsukino::Asset::ShaderAsset> vsAsset =
                std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assets->Get(ctx->builtinAssets->shaders.spriteVS));

            //-------------------------------------------------------------
            // PS
            //-------------------------------------------------------------
            std::shared_ptr<Tsukino::Asset::ShaderAsset> psAsset =
                std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assets->Get(ctx->builtinAssets->shaders.spritePS));

            // シェーダーアセットが両方とも有効なら
            if(vsAsset && psAsset) {
                //-------------------------------------------------------------
                // layout
                //-------------------------------------------------------------
                D3D11_INPUT_ELEMENT_DESC layout[] = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
                };
                // パイプラインを生成してキャッシュ
                m_pipelineCache =
                    ctx->renderer->GetPipelineFactory()->Create(*vsAsset, *psAsset, layout, ARRAYSIZE(layout), Tsukino::Renderer::DepthMode::None);
            }
        }

        if(!m_pipelineCache)
            return;    // パイプラインが作れなければ描画しない

        // 前フレームのマテリアルバッファをクリアして再利用
        m_materialBuffer.clear();

        // TransformComponent と SpriteComponent の両方を持つエンティティを取得
        auto view = registry.View<TransformComponent, SpriteComponent>();

        //-------------------------------------------------------------
        // 各エンティティから情報を抽出して描画コマンドを作成する
        //-------------------------------------------------------------
        view.each([&](entt::entity, const Tsukino::BuiltIn::ECS::TransformComponent& transform, const Tsukino::BuiltIn::ECS::SpriteComponent& sprite) {
            //-------------------------------------------------------------
            // テクスチャアセットからピクセルサイズを取得
            //-------------------------------------------------------------
            auto textureAsset = std::static_pointer_cast<Tsukino::Asset::TextureAsset>(ctx->assets->Get(sprite.textureHandle));
            if(!textureAsset)
                return;

            float texW = static_cast<float>(textureAsset->width);
            float texH = static_cast<float>(textureAsset->height);

            //-------------------------------------------------------------
            // テクスチャサイズ分だけ引き伸ばす行列を作成
            //-------------------------------------------------------------
            const auto pixelScaleMatrix = Tsukino::Core::Math::matrix::scale(texW, texH, 1.0f);

            Tsukino::Renderer::DrawCommand cmd;

            //TransformSystemが計算したworldMatrixの「手前」にピクセルスケールを噛ませる
            // 順序：(モデル) -> ピクセル拡大 -> TransformSystemの計算(S*R*T) -> (ワールド)
            cmd.transform = hlslpp::mul(transform.worldMatrix, pixelScaleMatrix);

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
            // バッファに実体を作成し、参照を取得 (std::dequeによりポインタは安全に保たれる)
            Tsukino::Renderer::Material& material = m_materialBuffer.emplace_back();

            //-------------------------------------------------------------
            // サンプラー設定
            //-------------------------------------------------------------
            material.SetSampler(ctx->renderer->GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp));    // リニアフィルタを指定

            //-------------------------------------------------------------
            // テクスチャ設定
            //-------------------------------------------------------------
            // ハンドルを取得
            Tsukino::Asset::AssetHandle handleId = sprite.textureHandle;

            // ハンドルがキャッシュにない場合はアセットマネージャーからテクスチャを取得してキャッシュに保存
            if(m_textureCache.find(handleId) == m_textureCache.end()) {
                Tsukino::Core::Ref<Tsukino::Asset::IAsset> asset = ctx->assets->Get(sprite.textureHandle);
                if(asset && asset->GetType() == Tsukino::Asset::AssetType::Texture) {
                    Tsukino::Core::Ref<Tsukino::Asset::TextureAsset> textureAsset = std::static_pointer_cast<Tsukino::Asset::TextureAsset>(asset);
                    m_textureCache[handleId]                                      = ctx->renderer->GetTextureSRV(*textureAsset);
                } else {
                    m_textureCache[handleId] = nullptr;
                }
            }
            material.SetTexture(m_textureCache[handleId]);

            //-------------------------------------------------------------
            // パイプライン設定
            //-------------------------------------------------------------
            material.SetPipeline(m_pipelineCache.get());

            // 実体のアドレスを DrawCommand に渡す
            cmd.material = &material;

            // コンテキストから取得したレンダラーを使って描画
            ctx->renderer->PushDrawCommand(cmd);
        });
    }
}    // namespace Tsukino::BuiltIn::ECS

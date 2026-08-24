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
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>

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

#include <algorithm>
#include <cmath>

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

        // 初回のみパイプラインを取得・生成する（Screen/World × Alpha/Additiveの4本）
        if(!m_pipelineCache || !m_additivePipelineCache || !m_worldPipelineCache || !m_worldAdditivePipelineCache) {
            std::shared_ptr<Tsukino::Asset::ShaderAsset> vsAsset =
                std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(ctx->builtinAssets->shaders.spriteVS));

            std::shared_ptr<Tsukino::Asset::ShaderAsset> psAsset =
                std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(ctx->builtinAssets->shaders.spritePS));

            std::shared_ptr<Tsukino::Asset::ShaderAsset> worldVsAsset =
                std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(ctx->assetManager->Get(ctx->builtinAssets->shaders.spriteWorldVS));

            // シェーダーアセットが両方とも有効なら
            if(vsAsset && psAsset) {
                //-------------------------------------------------------------
                // 生レイアウト配列を削除し、VertexFormat::Sprite を指定
                //-------------------------------------------------------------
                m_pipelineCache = ctx->renderer->GetPipelineFactory()->Create(*vsAsset,
                                                                              *psAsset,
                                                                              Tsukino::GraphicsCommon::VertexFormat::Sprite,    // 頂点フォーマットを指定
                                                                              Tsukino::Renderer::DepthMode::None,
                                                                              Tsukino::Renderer::BlendMode::Alpha);

                // 発光表現（EXP玉等）用。シェーダー自体はAlpha版と共通で、ブレンドステートだけ異なる
                m_additivePipelineCache = ctx->renderer->GetPipelineFactory()->Create(*vsAsset,
                                                                                      *psAsset,
                                                                                      Tsukino::GraphicsCommon::VertexFormat::Sprite,
                                                                                      Tsukino::Renderer::DepthMode::None,
                                                                                      Tsukino::Renderer::BlendMode::Additive);
            }

            // ワールド空間ビルボード用（SpriteSpace::World）。頂点シェーダーがY反転を行わない
            // spriteWorldVSを使い、G-Bufferが書いた深度に対してReadOnlyでテストする
            // （自身は深度を書かない＝奥から手前への並び替えはしないが、alpha/additiveの
            //   薄いエフェクト用途では十分。他の不透明オブジェクトの手前/後ろの判定は正しく出る）
            if(worldVsAsset && psAsset) {
                m_worldPipelineCache = ctx->renderer->GetPipelineFactory()->Create(*worldVsAsset,
                                                                                   *psAsset,
                                                                                   Tsukino::GraphicsCommon::VertexFormat::Sprite,
                                                                                   Tsukino::Renderer::DepthMode::ReadOnly,
                                                                                   Tsukino::Renderer::BlendMode::Alpha);

                m_worldAdditivePipelineCache = ctx->renderer->GetPipelineFactory()->Create(*worldVsAsset,
                                                                                           *psAsset,
                                                                                           Tsukino::GraphicsCommon::VertexFormat::Sprite,
                                                                                           Tsukino::Renderer::DepthMode::ReadOnly,
                                                                                           Tsukino::Renderer::BlendMode::Additive);
            }
        }

        if(!m_pipelineCache || !m_additivePipelineCache || !m_worldPipelineCache || !m_worldAdditivePipelineCache)
            return;    // パイプラインが作れなければ描画しない

        //-------------------------------------------------------------
        // World空間スプライト（ビルボード）が主カメラを向くための回転を求めておく。
        // WorldAnchorSystemと同じ探し方でisPrimaryなCameraComponentを1つ取得し、
        // viewMatrixの逆行列から位置・回転を分解する（decomposePositionRotationは
        // CombatSystem.cppのボーン姿勢取得等で既に使われている手法と同じ）
        //-------------------------------------------------------------
        bool               hasCamera = false;
        hlslpp::quaternion cameraRotation(0.0f, 0.0f, 0.0f, 1.0f);
        {
            auto cameraView = registry.View<CameraComponent>();
            for(auto entity : cameraView) {
                const auto& camera = cameraView.get<CameraComponent>(entity);
                if(camera.isPrimary) {
                    hlslpp::float3 cameraPosition;    // ここでは使わない（回転だけ取り出す）
                    Tsukino::Core::Math::matrix::decomposePositionRotation(
                        hlslpp::inverse(camera.viewMatrix), cameraPosition, cameraRotation);
                    hasCamera = true;
                    break;
                }
            }
        }

        // 前フレームのマテリアルバッファをクリアして再利用
        m_materialBuffer.clear();
        m_materialDataBuffer.clear();

        // まずエンティティ情報を一時バッファに収集（確保済み容量は維持して使い回す）
        m_entries.clear();

        // TransformComponent と SpriteComponent の両方を持つエンティティを取得
        auto view = registry.View<TransformComponent, SpriteComponent>();

        // 各エンティティから情報を抽出して描画コマンドを作成する
        view.each([&](entt::entity, const Tsukino::BuiltIn::ECS::TransformComponent& transform, const Tsukino::BuiltIn::ECS::SpriteComponent& sprite) {
            //-------------------------------------------------------------
            // スケールが潰れているスプライトは面積ゼロで、描いても1ピクセルも塗られない。
            // 敵の頭上HPバーは被弾していない間ずっと scale=0 で待機しているため、
            // 敵を大量に出すとここだけで敵数×2本の無駄なドローコールが積まれていた
            //-------------------------------------------------------------
            constexpr float kMinVisibleScale = 1.0e-4f;
            if(std::abs(transform.scale.x) < kMinVisibleScale || std::abs(transform.scale.y) < kMinVisibleScale)
                return;

            // World空間ビルボードは、主カメラが見つからないフレームは描きようがないのでスキップする
            // （Screen空間はカメラ不要のため対象外）
            if(sprite.space == Tsukino::BuiltIn::ECS::SpriteSpace::World && !hasCamera)
                return;

            std::shared_ptr<Tsukino::Asset::TextureAsset> textureAsset =
                std::static_pointer_cast<Tsukino::Asset::TextureAsset>(ctx->assetManager->Get(sprite.textureHandle));
            if(!textureAsset)
                return;

            float texW = static_cast<float>(textureAsset->width);
            float texH = static_cast<float>(textureAsset->height);

            // テクスチャサイズ分だけ引き伸ばす行列を作成
            const auto scaleMatrix = Tsukino::Core::Math::matrix::scale(texW * transform.scale.x, texH * transform.scale.y, 1.0f);

            Tsukino::Renderer::DrawCommand cmd;

            const auto translationMatrix = Tsukino::Core::Math::matrix::translate(transform.position);

            if(sprite.space == Tsukino::BuiltIn::ECS::SpriteSpace::World) {
                // 主カメラを向くビルボード：スケール→カメラ回転→ワールド位置への平行移動、の順で合成する
                // （他のTRS合成と同じくscale→rotate→translateの順。fromQuaternionは
                //   Matrix.hppにある既存ユーティリティ）
                const auto rotationMatrix = Tsukino::Core::Math::matrix::fromQuaternion(cameraRotation);
                cmd.transform              = hlslpp::mul(scaleMatrix, hlslpp::mul(rotationMatrix, translationMatrix));
            } else {
                // スケールを適用してから移動することで、Positionにはスケール倍率が掛からなくなります
                cmd.transform = hlslpp::mul(scaleMatrix, translationMatrix);
            }

            // メッシュの指定
            cmd.mesh = ctx->renderer->GetPrimitiveMesh(Tsukino::GraphicsCommon::PrimitiveType::Quad);

            // マテリアルの構築
            Tsukino::Renderer::Material& material = m_materialBuffer.emplace_back();

            // サンプラー設定
            material.SetSampler(ctx->renderer->GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp));

            // テクスチャ設定
            Tsukino::Asset::AssetHandle handleId = sprite.textureHandle;

            if(m_textureCache.find(handleId) == m_textureCache.end()) {
                Tsukino::Core::Ref<Tsukino::Asset::IAsset> asset = ctx->assetManager->Get(sprite.textureHandle);
                if(asset && asset->GetType() == Tsukino::Asset::AssetType::Texture) {
                    Tsukino::Core::Ref<Tsukino::Asset::TextureAsset> texAsset = std::static_pointer_cast<Tsukino::Asset::TextureAsset>(asset);
                    m_textureCache[handleId]                                  = ctx->renderer->GetTextureSRV(*texAsset);
                } else {
                    m_textureCache[handleId] = nullptr;
                }
            }
            material.SetTexture(m_textureCache[handleId]);

            // パイプライン設定（座標空間×合成方法の組み合わせで4通りから選ぶ）
            bool isWorld    = sprite.space == Tsukino::BuiltIn::ECS::SpriteSpace::World;
            bool isAdditive = sprite.blendMode == Tsukino::BuiltIn::ECS::SpriteBlendMode::Additive;
            if(isWorld)
                material.SetPipeline(isAdditive ? m_worldAdditivePipelineCache.get() : m_worldPipelineCache.get());
            else
                material.SetPipeline(isAdditive ? m_additivePipelineCache.get() : m_pipelineCache.get());

            // tintColorをb2(CBufferMaterial::baseColor)経由でSprite.ps.hlslへ渡す
            Tsukino::Renderer::CBufferMaterial& materialData = m_materialDataBuffer.emplace_back();
            materialData.baseColor                            = sprite.tintColor;

            cmd.material     = &material;
            cmd.materialData = &materialData;
            // World空間はG-Bufferが書いた深度と前後判定させたいのでRenderPass::World、
            // Screen空間（HUD）は従来どおり深度無視で最前面に出すOverlay
            cmd.pass = isWorld ? Tsukino::Renderer::RenderPass::World : Tsukino::Renderer::RenderPass::Overlay;

            m_entries.push_back({sprite.sortOrder, cmd});
        });

        // sortOrderで昇順ソート
        // （stable_sortにはしない。DrawCommandが16バイト境界を要求するため、
        //   一時バッファを使うstable_sortはaligned_storageの拡張アライメント検査に引っかかる）
        std::sort(m_entries.begin(), m_entries.end(), [](const SpriteEntry& a, const SpriteEntry& b) { return a.sortOrder < b.sortOrder; });
        // ソート済みの順でpush
        for(auto& e : m_entries) {
            ctx->renderer->PushDrawCommand(e.cmd);
        }
    }
}    // namespace Tsukino::BuiltIn::ECS

//-------------------------------------------------------------
//! @file   FontRendererSystem.cpp
//! @brief  FontRendererSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/BuiltInAssets.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Font/FontAsset.hpp>
#include <Tsukino/Engine/Asset/Font/DynamicFontAsset.hpp>

#include <Tsukino/Renderer/DrawCommand.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>

#include <SpriteFont.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @brief 更新関数
    //-------------------------------------------------------------
    void FontRendererSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();

        if(!ctx || !ctx->renderer)
            return;

        //-------------------------------------------------------------
        // スプライトバッチがない場合は作成(キャッシュ)
        //-------------------------------------------------------------
        if(!m_spriteBatch) {
            m_spriteBatch = ctx->renderer->CreateSpriteBatch();
        }

        // Transform と Font を両方持つエンティティを走査
        auto view = registry.View<TransformComponent, FontComponent>();

        view.each([&](entt::entity entity, const TransformComponent& transform, const FontComponent& font) {
            if(font.text.empty())
                return;

            Tsukino::Asset::AssetHandle fontHandle = font.fontHandle;

            // フォントハンドルが無効ならデフォルトフォントを使う
            if(!fontHandle.IsValid()) {
                fontHandle = ctx->builtinAssets->fonts.defaultFont;
            }

            Tsukino::Core::Ref<Tsukino::Asset::IAsset> asset = ctx->assetManager->Get(fontHandle);
            if(!asset)
                return;

            //-------------------------------------------------------------
            // Transformから情報を抽出
            //-------------------------------------------------------------
            hlslpp::float3 worldPos   = transform.worldMatrix[3].xyz;
            float          finalScale = hlslpp::length(transform.worldMatrix[0].xyz);

            DirectX::CommonStates* states = ctx->renderer->GetCommonStatesTK();

            //-------------------------------------------------------------
            // 描画コマンドの作成
            //-------------------------------------------------------------
            Tsukino::Renderer::DrawCommand cmd{};

            if(asset->GetType() == Tsukino::Asset::AssetType::DynamicFont) {
                //-------------------------------------------------------------
                // 動的フォントアトラス経路（オンデマンドグリフラスタライズ、日本語などに対応）
                //-------------------------------------------------------------
                auto& atlas = m_dynamicFontCache[fontHandle];

                if(!atlas) {
                    Tsukino::Core::Ref<Tsukino::Asset::DynamicFontAsset> dynamicAsset =
                        std::static_pointer_cast<Tsukino::Asset::DynamicFontAsset>(asset);

                    atlas = std::make_unique<Tsukino::Renderer::DynamicFontAtlas>(
                        ctx->renderer->GetDevice(), dynamicAsset->m_fontFileData.data(), dynamicAsset->m_fontFileData.size(), dynamicAsset->m_pixelSize);
                }

                // キャプチャ：DynamicFontAtlas の生ポインタを渡す
                Tsukino::Renderer::DynamicFontAtlas* nativeAtlas = atlas.get();

                cmd.customDraw =
                    [this, nativeAtlas, text = font.text, color = font.color, origin = font.origin, worldPos, finalScale, states](ID3D11DeviceContext* context) {
                        m_spriteBatch->Begin(DirectX::SpriteSortMode_Deferred,    // 描画順を自動整理
                                             states->NonPremultiplied(),          // 一般的なアルファブレンドを強制
                                             nullptr,                             // サンプラーステート（デフォルトでOK）
                                             states->DepthRead(),                 // 奥行きを読み取るけど書き込まない
                                             nullptr                              // ラスタライザステート
                        );

                        nativeAtlas->DrawString(
                            m_spriteBatch.get(), context, text, hlslpp::float2(worldPos.x, worldPos.y), color, origin, finalScale);

                        m_spriteBatch->End();
                    };
            } else {
                //-------------------------------------------------------------
                // 事前ベイクされたSpriteFont経路（既存）
                //-------------------------------------------------------------
                auto& spriteFont = m_fontCache[fontHandle];

                if(!spriteFont) {
                    Tsukino::Core::Ref<Tsukino::Asset::FontAsset> fontAsset = std::static_pointer_cast<Tsukino::Asset::FontAsset>(asset);

                    if(!fontAsset)
                        return;

                    //-------------------------------------------------------------
                    // フォントを生成してキャッシュに保存
                    //-------------------------------------------------------------
                    spriteFont = ctx->renderer->CreateSpriteFont(fontAsset->m_binaryData.data(), fontAsset->m_binaryData.size());
                }

                // キャプチャ：SpriteFont の生ポインタを渡す
                DirectX::SpriteFont* nativeFont = spriteFont.get();

                //-------------------------------------------------------------
                // フォントに存在しない文字をそのままDrawStringへ渡すと例外で落ちるため、
                // 事前にフォントが持つ文字だけへ安全側に置換する
                // (日本語未対応のベイク済みフォントに日本語テキストを渡した場合など)
                //-------------------------------------------------------------
                std::wstring safeText;
                safeText.reserve(font.text.size());
                for(wchar_t ch : font.text) {
                    if(nativeFont->ContainsCharacter(ch)) {
                        safeText.push_back(ch);
                    } else if(nativeFont->ContainsCharacter(L'?')) {
                        safeText.push_back(L'?');
                    }
                }

                if(safeText.empty())
                    return;

                cmd.customDraw =
                    [this, nativeFont, text = std::move(safeText), color = font.color, origin = font.origin, worldPos, finalScale, states](
                        ID3D11DeviceContext* context) {
                        m_spriteBatch->Begin(DirectX::SpriteSortMode_Deferred,    // 描画順を自動整理
                                             states->NonPremultiplied(),          // 一般的なアルファブレンドを強制
                                             nullptr,                             // サンプラーステート（デフォルトでOK）
                                             states->DepthRead(),                 // 奥行きを読み取るけど書き込まない
                                             nullptr                              // ラスタライザステート
                        );

                        DirectX::XMFLOAT4 dxColor(color.x, color.y, color.z, color.w);
                        nativeFont->DrawString(m_spriteBatch.get(),
                                               text.c_str(),
                                               DirectX::XMFLOAT2(worldPos.x, worldPos.y),
                                               DirectX::XMLoadFloat4(&dxColor),
                                               0.0f,
                                               DirectX::XMFLOAT2(origin.x, origin.y),
                                               finalScale);

                        m_spriteBatch->End();
                    };
            }

            // UIはトーンマップ後のバックバッファへ描く（SpriteRendererSystemと揃える）。
            // これを設定しないと既定のRenderPass::Worldに積まれ、3Dモデル(Render)より先に描かれて
            // 後から上書きされてしまう上、トーンマップ前のHDRターゲットに描画されて発色もずれる
            cmd.pass = Tsukino::Renderer::RenderPass::Overlay;

            ctx->renderer->PushDrawCommand(cmd);
        });
    }

}    // namespace Tsukino::BuiltIn::ECS

//-------------------------------------------------------------
//! @file   FontRendererSystem.cpp
//! @brief  FontRendererSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/WorldAnchorComponent.hpp>
#include <Tsukino/BuiltIn/BuiltInAssets.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Font/FontAsset.hpp>
#include <Tsukino/Engine/Asset/Font/DynamicFontAsset.hpp>

#include <Tsukino/Renderer/DrawCommand.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>

#include <SpriteFont.h>

#include <algorithm>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    namespace {
        //-------------------------------------------------------------
        //! @brief 文字列の計測が必要かを返す関数
        //! @note  既定の揃え(Left/Top)では原点が変わらないため、計測そのものを省ける
        //-------------------------------------------------------------
        [[nodiscard]]
        bool NeedsMeasure(HorizontalAlign horizontal, VerticalAlign vertical) {
            return horizontal != HorizontalAlign::Left || vertical != VerticalAlign::Top;
        }

        //-------------------------------------------------------------
        //! @brief 揃え設定から原点（スケール適用前）を求める関数
        //! @param textSize [in] 文字列の描画サイズ（スケール適用前）
        //-------------------------------------------------------------
        [[nodiscard]]
        hlslpp::float2 ResolveAlignedOrigin(hlslpp::float2 baseOrigin, hlslpp::float2 textSize, HorizontalAlign horizontal, VerticalAlign vertical) {
            hlslpp::float2 origin = baseOrigin;

            switch(horizontal) {
            case HorizontalAlign::Center: origin.x += textSize.x * 0.5f; break;
            case HorizontalAlign::Right:  origin.x += textSize.x; break;
            case HorizontalAlign::Left:   break;
            }

            switch(vertical) {
            case VerticalAlign::Middle: origin.y += textSize.y * 0.5f; break;
            case VerticalAlign::Bottom: origin.y += textSize.y; break;
            case VerticalAlign::Top:    break;
            }

            return origin;
        }
    }    // namespace

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

        ID3D11DeviceContext* immediateContext = ctx->renderer->GetContext();

        //-------------------------------------------------------------
        // 1フレーム分の描画情報を集め直す。
        // clear()は確保済みの容量を維持するため、ウォームアップ後は追加確保が起きない
        //-------------------------------------------------------------
        m_drawEntries.clear();

        // Transform と Font を両方持つエンティティを走査
        auto view = registry.View<TransformComponent, FontComponent>();

        view.each([&](entt::entity entity, const TransformComponent& transform, const FontComponent& font) {
            if(font.text.empty())
                return;

            //-------------------------------------------------------------
            // WorldAnchorComponentを持つ場合、画面内に投影できなかったフレームは描かない。
            // WorldAnchorSystemはvisible=falseのときpositionを更新しないため、
            // これが無いと対象がカメラ後方へ回った瞬間の座標に文字が取り残される
            //-------------------------------------------------------------
            if(const auto* anchor = registry.try_get<WorldAnchorComponent>(entity); anchor && !anchor->visible)
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

                DrawEntry& entry  = m_drawEntries.emplace_back();
                entry.atlas       = nativeAtlas;
                entry.text        = font.text;
                entry.position    = hlslpp::float2(worldPos.x, worldPos.y);
                entry.origin      = NeedsMeasure(font.horizontalAlign, font.verticalAlign)
                                        ? ResolveAlignedOrigin(font.origin,
                                                               nativeAtlas->MeasureString(font.text, immediateContext),
                                                               font.horizontalAlign,
                                                               font.verticalAlign)
                                        : font.origin;
                entry.color        = font.color;
                entry.outlineColor = font.outlineColor;
                entry.outlineWidth = font.outlineWidth;
                entry.scale        = finalScale;
                entry.sortOrder    = font.sortOrder;
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

                hlslpp::float2 alignedOrigin = font.origin;
                if(NeedsMeasure(font.horizontalAlign, font.verticalAlign)) {
                    DirectX::XMFLOAT2 measured{};
                    DirectX::XMStoreFloat2(&measured, nativeFont->MeasureString(safeText.c_str()));

                    alignedOrigin =
                        ResolveAlignedOrigin(font.origin, hlslpp::float2(measured.x, measured.y), font.horizontalAlign, font.verticalAlign);
                }

                DrawEntry& entry   = m_drawEntries.emplace_back();
                entry.spriteFont   = nativeFont;
                entry.text         = std::move(safeText);
                entry.position     = hlslpp::float2(worldPos.x, worldPos.y);
                entry.origin       = alignedOrigin;
                entry.color        = font.color;
                entry.outlineColor = font.outlineColor;
                entry.outlineWidth = font.outlineWidth;
                entry.scale        = finalScale;
                entry.sortOrder    = font.sortOrder;
            }
        });

        if(m_drawEntries.empty())
            return;

        //-------------------------------------------------------------
        // sortOrderの昇順に並べ替える。同じsortOrder同士の順序を保つため安定ソートを使う
        // （安定ソートでないとEnTTのプール順の揺れが描画順の揺れとして現れる）。
        // DrawEntryそのものではなく添字を並べ替えるのは、DrawEntryがhlsl++の
        // 16バイトアラインメント型を含み、std::stable_sortの一時バッファが
        // 拡張アラインメントとして弾かれるため
        //-------------------------------------------------------------
        m_drawOrder.clear();
        m_drawOrder.reserve(m_drawEntries.size());
        for(std::uint32_t index = 0; index < static_cast<std::uint32_t>(m_drawEntries.size()); ++index) {
            m_drawOrder.push_back(index);
        }

        std::stable_sort(m_drawOrder.begin(), m_drawOrder.end(), [this](std::uint32_t lhs, std::uint32_t rhs) {
            return m_drawEntries[lhs].sortOrder < m_drawEntries[rhs].sortOrder;
        });

        DirectX::CommonStates* states = ctx->renderer->GetCommonStatesTK();

        //-------------------------------------------------------------
        // 描画コマンドの作成。全エンティティ分をBegin/End 1組にまとめる
        //-------------------------------------------------------------
        Tsukino::Renderer::DrawCommand cmd{};

        // m_drawEntriesはthis経由で参照する。customDrawが呼ばれるのはRenderer::Render()の中で、
        // これは同フレームのUpdateが全て終わった後（EngineAPI::Update → EngineAPI::Render）なので、
        // 描画時点の内容は今フレーム収集したものと一致する
        cmd.customDraw = [this, states](ID3D11DeviceContext* context) {
            m_spriteBatch->Begin(DirectX::SpriteSortMode_Deferred,    // 積んだ順に描く
                                 states->NonPremultiplied(),          // 一般的なアルファブレンドを強制
                                 nullptr,                             // サンプラーステート（デフォルトでOK）
                                 states->DepthRead(),                 // 奥行きを読み取るけど書き込まない
                                 nullptr                              // ラスタライザステート
            );

            for(std::uint32_t index : m_drawOrder) {
                const DrawEntry& entry = m_drawEntries[index];

                if(entry.atlas) {
                    entry.atlas->DrawString(m_spriteBatch.get(), context, entry.text, entry.position, entry.color, entry.origin, entry.scale,
                                            entry.outlineColor, entry.outlineWidth);
                    continue;
                }

                //-------------------------------------------------------------
                // ベイク済みSpriteFont経路。縁取りはDynamicFontAtlasと揃えて
                // 8方向へずらしたものを先に描き、最後に本体を描く
                //-------------------------------------------------------------
                DirectX::XMFLOAT2 origin(entry.origin.x, entry.origin.y);

                if(entry.outlineWidth > 0.0f && entry.outlineColor.w > 0.0f) {
                    constexpr float kOutlineOffsets[8][2] = {
                        {-1.0f, -1.0f}, {0.0f, -1.0f}, {1.0f, -1.0f},
                        {-1.0f,  0.0f},                {1.0f,  0.0f},
                        {-1.0f,  1.0f}, {0.0f,  1.0f}, {1.0f,  1.0f},
                    };

                    DirectX::XMFLOAT4 dxOutlineColor(entry.outlineColor.x, entry.outlineColor.y, entry.outlineColor.z, entry.outlineColor.w);
                    DirectX::XMVECTOR outlineColorVec = DirectX::XMLoadFloat4(&dxOutlineColor);

                    for(const auto& offset : kOutlineOffsets) {
                        entry.spriteFont->DrawString(
                            m_spriteBatch.get(),
                            entry.text.c_str(),
                            DirectX::XMFLOAT2(entry.position.x + offset[0] * entry.outlineWidth, entry.position.y + offset[1] * entry.outlineWidth),
                            outlineColorVec,
                            0.0f,
                            origin,
                            entry.scale);
                    }
                }

                DirectX::XMFLOAT4 dxColor(entry.color.x, entry.color.y, entry.color.z, entry.color.w);
                entry.spriteFont->DrawString(m_spriteBatch.get(),
                                             entry.text.c_str(),
                                             DirectX::XMFLOAT2(entry.position.x, entry.position.y),
                                             DirectX::XMLoadFloat4(&dxColor),
                                             0.0f,
                                             origin,
                                             entry.scale);
            }

            m_spriteBatch->End();
        };

        // UIはトーンマップ後のバックバッファへ描く（SpriteRendererSystemと揃える）。
        // これを設定しないと既定のRenderPass::Worldに積まれ、3Dモデル(Render)より先に描かれて
        // 後から上書きされてしまう上、トーンマップ前のHDRターゲットに描画されて発色もずれる
        cmd.pass = Tsukino::Renderer::RenderPass::Overlay;

        ctx->renderer->PushDrawCommand(cmd);
    }

}    // namespace Tsukino::BuiltIn::ECS

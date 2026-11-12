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
#include <Tsukino/BuiltIn/ECS/UI/UICanvas.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UICanvasGroupUtility.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UIClipUtility.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UIRectUtility.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UIVisibilityUtility.hpp>
#include <Tsukino/BuiltIn/BuiltInAssets.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Font/FontAsset.hpp>
#include <Tsukino/Engine/Asset/Font/DynamicFontAsset.hpp>

#include <Tsukino/Renderer/DrawCommand.hpp>
#include <Tsukino/Renderer/DrawCommandQueue.hpp>
#include <Tsukino/Renderer/Text/BakedFont.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>



#include <algorithm>
#include <cmath>
#include <cwctype>
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

        //-------------------------------------------------------------
        //! 枠に収まるようにスケールを縮めます。
        //! @param  [in] scale         Transformから求めたスケール
        //! @param  [in] measuredWidth スケール適用前の文字列の幅
        //! @param  [in] maxWidth      描画幅の上限（ピクセル。0以下なら制限なし）
        //! @return 描画に使うスケール
        //-------------------------------------------------------------
        [[nodiscard]]
        float FitScale(float scale, float measuredWidth, float maxWidth) {
            if(maxWidth <= 0.0f || measuredWidth <= 0.0f)
                return scale;
            const float width = measuredWidth * scale;
            return (width > maxWidth) ? scale * (maxWidth / width) : scale;
        }

        //-------------------------------------------------------------
        //! 揃えの割合（左・上 0、中央 0.5、右・下 1）を返します。
        //-------------------------------------------------------------
        [[nodiscard]]
        hlslpp::float2 AlignFactor(HorizontalAlign horizontal, VerticalAlign vertical) {
            const float x = (horizontal == HorizontalAlign::Center) ? 0.5f : (horizontal == HorizontalAlign::Right) ? 1.0f : 0.0f;
            const float y = (vertical == VerticalAlign::Middle) ? 0.5f : (vertical == VerticalAlign::Bottom) ? 1.0f : 0.0f;
            return hlslpp::float2(x, y);
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
        // 1フレーム分の描画情報を集め直す。
        // clear()は確保済みの容量を維持するため、ウォームアップ後は追加確保が起きない
        //-------------------------------------------------------------
        m_drawEntries.clear();

        // UI の座標と画面のピクセルの対応。文字は UI のカメラを通らずスプライトとしてピクセルで描くので、
        // 位置・大きさ・縁の太さ・切り取り枠をここでピクセルにする（大きさを掛けてから文字の画像を作るので、拡大してもぼやけない）
        const UICanvas& canvas = GetUICanvas(registry);

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

            // 自身か祖先の UIVisibilityComponent で隠されている
            if(UIVisibilityUtility::IsHidden(registry, entity))
                return;

            // 自身と祖先の UICanvasGroupComponent の不透明度。ほぼ透明なら描かない
            const float groupAlpha = UICanvasGroupUtility::GetAlpha(registry, entity);
            if(groupAlpha < UICanvasGroupUtility::kInvisibleAlpha)
                return;

            //-------------------------------------------------------------
            // 祖先の UIClipComponent の枠（スクロールする UI の枠など）。描くときにシザーで切る
            //-------------------------------------------------------------
            bool                        hasClip = false;
            Tsukino::Renderer::ClipRect clipRect;
            UIClipUtility::ClipBounds   clipBounds;
            if(UIClipUtility::TryGetClipBounds(registry, entity, clipBounds)) {
                const UIClipUtility::ClipBounds pixel = canvas.ToPixel(clipBounds);
                hasClip                               = true;
                clipRect.left                         = static_cast<i32>(std::floor(pixel.left));
                clipRect.top                          = static_cast<i32>(std::floor(pixel.top));
                clipRect.right                        = static_cast<i32>(std::ceil(pixel.right));
                clipRect.bottom                       = static_cast<i32>(std::ceil(pixel.bottom));
            }

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
            const float    worldScale = hlslpp::length(transform.worldMatrix[0].xyz);
            float          finalScale = worldScale * font.scale;

            //-------------------------------------------------------------
            // UIRectComponent があれば、揃えの基準点を四角の中の点にする（左揃えなら左端、中央なら中心…）。
            // 収める幅を指定していなければ四角の幅に収める
            //-------------------------------------------------------------
            float maxWidth = font.maxWidth;
            if(const auto* rect = registry.try_get<UIRectComponent>(entity)) {
                const UIRectUtility::LocalRect local = UIRectUtility::FromRect(*rect);
                const hlslpp::float2           point = local.center + (AlignFactor(font.horizontalAlign, font.verticalAlign) - 0.5f) * local.size;
                const hlslpp::float2           world = UIRectUtility::LocalToWorld(transform.worldMatrix, point);
                worldPos.x                           = world.x;
                worldPos.y                           = world.y;
                if(maxWidth <= 0.0f)
                    maxWidth = std::abs(float(rect->size.x)) * worldScale;
            }

            // 画面上の回転（Z 回転を親まで合成したもの）。行ベクトル規約なので1行目が回した X 軸の向き
            const float rotation = std::atan2(float(transform.worldMatrix[0].y), float(transform.worldMatrix[0].x));

            if(asset->GetType() == Tsukino::Asset::AssetType::DynamicFont) {
                //-------------------------------------------------------------
                // 動的フォントアトラス経路（オンデマンドグリフラスタライズ、日本語などに対応）
                //-------------------------------------------------------------
                auto& atlas = m_dynamicFontCache[fontHandle];

                if(!atlas) {
                    Tsukino::Core::Ref<Tsukino::Asset::DynamicFontAsset> dynamicAsset =
                        std::static_pointer_cast<Tsukino::Asset::DynamicFontAsset>(asset);

                    atlas = std::make_unique<Tsukino::Renderer::DynamicFontAtlas>(
                        ctx->renderer->GetResources(), dynamicAsset->m_fontFileData.data(), dynamicAsset->m_fontFileData.size(), dynamicAsset->m_pixelSize);
                }

                // 描画情報には DynamicFontAtlas の生ポインタを持たせる（キャッシュが所有する）
                Tsukino::Renderer::DynamicFontAtlas* nativeAtlas = atlas.get();

                // 揃え位置と枠に収める計算のどちらかが要るときだけ測る
                const bool     needsMeasure = NeedsMeasure(font.horizontalAlign, font.verticalAlign) || maxWidth > 0.0f;
                hlslpp::float2 measured(0.0f, 0.0f);
                if(needsMeasure)
                    measured = nativeAtlas->MeasureString(font.text);

                DrawEntry& entry  = m_drawEntries.emplace_back();
                entry.atlas       = nativeAtlas;
                entry.text        = font.text;
                entry.position    = canvas.ToPixel(hlslpp::float2(worldPos.x, worldPos.y));
                entry.origin      = NeedsMeasure(font.horizontalAlign, font.verticalAlign)
                                        ? ResolveAlignedOrigin(font.origin, measured, font.horizontalAlign, font.verticalAlign)
                                        : font.origin;
                entry.color        = font.color;
                entry.outlineColor = font.outlineColor;
                entry.color.w *= groupAlpha;
                entry.outlineColor.w *= groupAlpha;
                entry.outlineWidth = font.outlineWidth * canvas.scale;
                entry.scale        = FitScale(finalScale, float(measured.x), maxWidth) * canvas.scale;    // 収める幅は UI の単位で計算してからピクセルにする
                entry.rotation     = rotation;
                entry.sortOrder    = font.sortOrder;
                entry.hasClip      = hasClip;
                entry.clipRect     = clipRect;
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
                    spriteFont = Tsukino::Renderer::BakedFont::Create(ctx->renderer->GetResources(), fontAsset->m_binaryData.data(),
                                                                      fontAsset->m_binaryData.size());
                    if(!spriteFont)
                        return;
                }

                // 描画情報にはフォントの生ポインタを持たせる（キャッシュが所有する）
                const Tsukino::Renderer::BakedFont* nativeFont = spriteFont.get();

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

                hlslpp::float2    alignedOrigin = font.origin;
                hlslpp::float2    measured(0.0f, 0.0f);
                if(NeedsMeasure(font.horizontalAlign, font.verticalAlign) || maxWidth > 0.0f)
                    measured = nativeFont->MeasureString(safeText);
                if(NeedsMeasure(font.horizontalAlign, font.verticalAlign)) {
                    alignedOrigin =
                        ResolveAlignedOrigin(font.origin, measured, font.horizontalAlign, font.verticalAlign);
                }

                DrawEntry& entry   = m_drawEntries.emplace_back();
                entry.spriteFont   = nativeFont;
                entry.text         = std::move(safeText);
                entry.position     = canvas.ToPixel(hlslpp::float2(worldPos.x, worldPos.y));
                entry.origin       = alignedOrigin;
                entry.color        = font.color;
                entry.outlineColor = font.outlineColor;
                entry.color.w *= groupAlpha;
                entry.outlineColor.w *= groupAlpha;
                entry.outlineWidth = font.outlineWidth * canvas.scale;
                entry.scale        = FitScale(finalScale, float(measured.x), maxWidth) * canvas.scale;
                entry.rotation     = rotation;
                entry.sortOrder    = font.sortOrder;
                entry.hasClip      = hasClip;
                entry.clipRect     = clipRect;
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

        //-------------------------------------------------------------
        // 描画コマンドの作成。sortOrderが同じ連なり（＝同じUIの層）ごとに1個ずつ発行する。
        //
        // 全文字を1個にまとめてしまうと、その1個がOverlay内で1つの層としてしか
        // 扱えず、「HPバーの上・スキルカードの下」のような層をまたぐ配置ができない。
        // 層ごとに分けても、同じ層の文字は1つのスプライトの並びとしてまとめて描かれるため
        // バッチングは保たれる（コマンド数は文字数ではなく層の数で決まる）。
        //
        // スプライトはここで作ってキューへ写すので、描画時にこのシステムの状態は読まれない
        //-------------------------------------------------------------
        auto sameClip = [](const DrawEntry& lhs, const DrawEntry& rhs) {
            if(lhs.hasClip != rhs.hasClip)
                return false;
            return !lhs.hasClip || (lhs.clipRect.left == rhs.clipRect.left && lhs.clipRect.top == rhs.clipRect.top && lhs.clipRect.right == rhs.clipRect.right
                                    && lhs.clipRect.bottom == rhs.clipRect.bottom);
        };

        Tsukino::Renderer::DrawCommandQueue& queue = ctx->renderer->GetDrawQueue();

        std::uint32_t runBegin = 0;
        while(runBegin < static_cast<std::uint32_t>(m_drawOrder.size())) {
            const DrawEntry& first = m_drawEntries[m_drawOrder[runBegin]];
            const int        layer = first.sortOrder;

            // 同じ層でも切り取り枠が違う文字は同じコマンドにまとめられない（切り取り枠は1コマンドに1つ）ので、そこでも区切る
            std::uint32_t runEnd = runBegin + 1;
            while(runEnd < static_cast<std::uint32_t>(m_drawOrder.size()) && m_drawEntries[m_drawOrder[runEnd]].sortOrder == layer
                  && sameClip(m_drawEntries[m_drawOrder[runEnd]], first))
                ++runEnd;

            Tsukino::Renderer::DrawCommand cmd{};

            // UIはトーンマップ後のバックバッファへ描く（SpriteRendererSystemと揃える）。
            // これを設定しないと既定のRenderPass::Worldに積まれ、3Dモデル(Render)より先に描かれて
            // 後から上書きされてしまう上、トーンマップ前のHDRターゲットに描画されて発色もずれる
            cmd.pass        = Tsukino::Renderer::RenderPass::Overlay;
            cmd.sortOrder   = layer;
            cmd.hasClipRect = first.hasClip;
            cmd.clipRect    = first.clipRect;

            m_sprites.clear();
            BuildSprites(runBegin, runEnd);
            queue.PushSprites(cmd, m_sprites);

            runBegin = runEnd;
        }
    }

    //-------------------------------------------------------------
    //! @brief m_drawOrderの一部区間の文字をスプライトにしてm_spritesへ並べる関数
    //-------------------------------------------------------------
    void FontRendererSystem::BuildSprites(std::uint32_t beginIndex, std::uint32_t endIndex) {
        for(std::uint32_t order = beginIndex; order < endIndex; ++order) {
            const DrawEntry& entry = m_drawEntries[m_drawOrder[order]];

            if(entry.atlas) {
                entry.atlas->AppendStringSprites(m_sprites, entry.text, entry.position, entry.color, entry.origin, entry.scale,
                                                 entry.outlineColor, entry.outlineWidth, entry.rotation);
                continue;
            }

            //-------------------------------------------------------------
            // ベイク済みSpriteFont経路。縁取りはDynamicFontAtlasと揃えて
            // 8方向へずらしたものを先に並べ、最後に本体を並べる
            //-------------------------------------------------------------
            if(entry.outlineWidth > 0.0f && entry.outlineColor.w > 0.0f) {
                constexpr float kOutlineOffsets[8][2] = {
                    {-1.0f, -1.0f}, {0.0f, -1.0f}, {1.0f, -1.0f},
                    {-1.0f,  0.0f},                {1.0f,  0.0f},
                    {-1.0f,  1.0f}, {0.0f,  1.0f}, {1.0f,  1.0f},
                };

                for(const auto& offset : kOutlineOffsets) {
                    const hlslpp::float2 shifted(entry.position.x + offset[0] * entry.outlineWidth, entry.position.y + offset[1] * entry.outlineWidth);
                    entry.spriteFont->AppendStringSprites(m_sprites, entry.text, shifted, entry.outlineColor, entry.rotation, entry.origin, entry.scale);
                }
            }

            entry.spriteFont->AppendStringSprites(m_sprites, entry.text, entry.position, entry.color, entry.rotation, entry.origin, entry.scale);
        }
    }

}    // namespace Tsukino::BuiltIn::ECS

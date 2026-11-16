//------------------------------------------------------------
//! @file	DynamicFontAtlas.cpp
//! @brief	グリフのオンデマンドラスタライズ（stb_truetype）と動的テクスチャアトラスの実装
//! @author 山﨑愛
//------------------------------------------------------------
#include <Tsukino/Renderer/Text/DynamicFontAtlas.hpp>
#include <Tsukino/Renderer/RenderResources.hpp>

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Utf8.hpp>

// stb_truetype の実装はこのファイルの中だけに置く（STBTT_STATIC。ほかのライブラリの同じ実装とぶつからない）
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>

#include <algorithm>
#include <cmath>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------
    //! フォントのデータと stb_truetype の状態です。
    //! stbtt_fontinfo はデータを指すだけなので、データもここで持つ。
    //------------------------------------------------------------
    struct DynamicFontAtlas::Font {
        std::vector<uint8_t> data;    // ttf/otf の中身
        stbtt_fontinfo       info{};  // stb_truetype がフォントを読むための状態
    };

    //------------------------------------------------------------
    //! @brief  コンストラクタ
    //------------------------------------------------------------
    DynamicFontAtlas::DynamicFontAtlas(RenderResources& resources, const uint8_t* fontData, size_t fontDataSize, float pixelSize)
        : m_resources(&resources), m_pixelSize(pixelSize) {
        auto font = std::make_unique<Font>();
        font->data.assign(fontData, fontData + fontDataSize);

        // TrueType（glyf）と CFF のどちらの .otf も読める
        const int offset = stbtt_GetFontOffsetForIndex(font->data.data(), 0);
        if(offset < 0 || !stbtt_InitFont(&font->info, font->data.data(), offset)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Unsupported font file.");
            return;
        }

        //--------------------------------------------------------------
        // メトリクスからピクセル換算スケールと行送り量を計算する。
        // 大きさは em の大きさで指定する（ascent + descent の高さではない）
        //--------------------------------------------------------------
        int ascent = 0, descent = 0, lineGap = 0;
        stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &lineGap);

        m_scale      = stbtt_ScaleForMappingEmToPixels(&font->info, pixelSize);
        m_lineHeight = static_cast<float>(ascent - descent + lineGap) * m_scale;    // descent は負の値
        m_ascent     = static_cast<float>(ascent) * m_scale;
        m_font       = std::move(font);
    }

    //------------------------------------------------------------
    //! @brief  デストラクタ
    //------------------------------------------------------------
    DynamicFontAtlas::~DynamicFontAtlas() = default;

    //------------------------------------------------------------
    //! @brief  新しいアトラスページを作成する関数
    //------------------------------------------------------------
    bool DynamicFontAtlas::CreatePage() {
        // 中身を渡さないと 0 で埋めて作られる
        // (グリフを書き込んでいない領域のゴミを、グリフ端のサンプリングで拾ってしまうのを防ぐ)
        Page page;
        page.texture = m_resources->CreateTexture(kPageSize, kPageSize, Tsukino::GraphicsCommon::TextureFormat::RGBA8, nullptr, 0);
        if(!page.texture.IsValid()) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to create atlas page texture.");
            return false;
        }

        m_pages.push_back(page);
        return true;
    }

    //------------------------------------------------------------
    //! @brief  指定サイズの矩形をアトラスに確保する関数
    //------------------------------------------------------------
    bool DynamicFontAtlas::AllocateRect(uint32_t width, uint32_t height, int& outPage, uint32_t& outX, uint32_t& outY) {
        // 隣のグリフとの間に余白を確保する。余白が無いとグリフ同士のインクが真横で接し、
        // バイリニアサンプリングが矩形の外＝隣のグリフの列を拾って縦棒状のゴミになる
        const uint32_t paddedWidth  = width + kGlyphPadding;
        const uint32_t paddedHeight = height + kGlyphPadding;

        if(paddedWidth > kPageSize || paddedHeight > kPageSize) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Glyph too large for atlas page.");
            return false;
        }

        if(m_pages.empty() && !CreatePage())
            return false;

        Page* page = &m_pages.back();

        // 現在のシェルフに収まらなければ次のシェルフへ折り返す
        if(page->cursorX + paddedWidth > kPageSize) {
            page->cursorX     = 0;
            page->cursorY += page->shelfHeight;
            page->shelfHeight = 0;
        }

        // 現在のページに収まらなければ新しいページを作成する
        if(page->cursorY + paddedHeight > kPageSize) {
            if(!CreatePage())
                return false;
            page = &m_pages.back();
        }

        outPage = static_cast<int>(m_pages.size() - 1);
        outX    = page->cursorX;
        outY    = page->cursorY;

        page->cursorX += paddedWidth;
        page->shelfHeight = std::max(page->shelfHeight, paddedHeight);

        return true;
    }

    //------------------------------------------------------------
    //! 文字のメトリクスをキャッシュから取得し、無ければフォントから読み込んでキャッシュします。
    //------------------------------------------------------------
    const DynamicFontAtlas::GlyphMetrics& DynamicFontAtlas::GetGlyphMetrics(char32_t codepoint) {
        const uint32_t key = static_cast<uint32_t>(codepoint);
        if(auto it = m_metricsCache.find(key); it != m_metricsCache.end())
            return it->second;

        GlyphMetrics metrics{};
        metrics.glyphIndex = stbtt_FindGlyphIndex(&m_font->info, static_cast<int>(codepoint));

        // 文字送りはフォントの設計値から求める（描く大きさによらず、位置がずれない）
        int advanceWidth = 0, leftSideBearing = 0;
        stbtt_GetGlyphHMetrics(&m_font->info, metrics.glyphIndex, &advanceWidth, &leftSideBearing);
        metrics.advanceX = static_cast<float>(advanceWidth) * m_scale;

        auto [it, _] = m_metricsCache.emplace(key, metrics);
        return it->second;
    }

    //------------------------------------------------------------
    //! 描く大きさから、ラスタライズする大きさ（段に切り上げた値）を求めます。
    //------------------------------------------------------------
    int DynamicFontAtlas::ToRasterSize(float drawSize) {
        if(drawSize <= static_cast<float>(kMinRasterSize)) {
            return kMinRasterSize;
        }
        // 浮動小数の誤差で、ちょうど段の大きさが1つ上の段へ繰り上がらないよう僅かに引く
        const float steps = std::ceil(std::log(drawSize / kRasterSizeBase) / std::log(kRasterSizeStep) - 1e-4f);
        const float size  = std::ceil(kRasterSizeBase * std::pow(kRasterSizeStep, steps) - 1e-3f);
        return std::clamp(static_cast<int>(size), kMinRasterSize, kMaxRasterSize);
    }

    //------------------------------------------------------------
    //! グリフを指定の大きさでキャッシュから取得し、無ければラスタライズしてキャッシュします。
    //------------------------------------------------------------
    const DynamicFontAtlas::GlyphImage& DynamicFontAtlas::GetOrRasterizeGlyph(char32_t codepoint, int rasterSize) {
        const uint64_t key = (static_cast<uint64_t>(rasterSize) << 32) | static_cast<uint32_t>(codepoint);
        if(auto it = m_glyphCache.find(key); it != m_glyphCache.end())
            return it->second;

        GlyphImage          image{};
        const GlyphMetrics& metrics = GetGlyphMetrics(codepoint);

        //--------------------------------------------------------------
        // グレースケールのアンチエイリアスでラスタライズする（ヒンティングはしない。
        // 大きさごとに字形が崩れて見えないように、どの大きさも同じ形のまま縮める）。
        // 矩形はベースラインのペン位置からの相対（y は下向き）
        //--------------------------------------------------------------
        const float rasterScale = stbtt_ScaleForMappingEmToPixels(&m_font->info, static_cast<float>(rasterSize));
        int         x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        stbtt_GetGlyphBitmapBox(&m_font->info, metrics.glyphIndex, rasterScale, rasterScale, &x0, &y0, &x1, &y1);

        const uint32_t width  = static_cast<uint32_t>(std::max(0, x1 - x0));
        const uint32_t height = static_cast<uint32_t>(std::max(0, y1 - y0));

        // インクの無いグリフ(全角スペース・結合文字等)は位置情報だけキャッシュする
        if(width == 0 || height == 0) {
            auto [it, _] = m_glyphCache.emplace(key, image);
            return it->second;
        }

        std::vector<uint8_t> coverage(static_cast<size_t>(width) * height);
        stbtt_MakeGlyphBitmap(&m_font->info, coverage.data(), static_cast<int>(width), static_cast<int>(height), static_cast<int>(width),
                              rasterScale, rasterScale, metrics.glyphIndex);

        // 白RGB + カバレッジ値のアルファへ展開する
        // (スプライトのシェーダー（SpriteBatch.ps.sc）はRGBA全チャンネルをサンプルして頂点色を乗算するため、
        //  単チャンネルフォーマットのままではカラーが正しく乗らない)
        std::vector<uint8_t> rgbaBuffer(static_cast<size_t>(width) * height * 4);
        for(size_t i = 0; i < static_cast<size_t>(width) * height; ++i) {
            rgbaBuffer[i * 4 + 0] = 255;
            rgbaBuffer[i * 4 + 1] = 255;
            rgbaBuffer[i * 4 + 2] = 255;
            rgbaBuffer[i * 4 + 3] = coverage[i];
        }

        int      pageIndex = 0;
        uint32_t atlasX = 0, atlasY = 0;
        if(!AllocateRect(width, height, pageIndex, atlasX, atlasY)) {
            auto [it, _] = m_glyphCache.emplace(key, image);
            return it->second;
        }

        m_resources->UpdateTexture(m_pages[pageIndex].texture, atlasX, atlasY, width, height, rgbaBuffer.data(), width * 4);

        image.hasInk    = true;
        image.page      = pageIndex;
        image.atlasRect = TexelRect{static_cast<i32>(atlasX), static_cast<i32>(atlasY), static_cast<i32>(atlasX + width), static_cast<i32>(atlasY + height)};
        image.bearingX  = static_cast<float>(x0);
        image.bearingY  = static_cast<float>(y0);

        auto [it, _] = m_glyphCache.emplace(key, image);
        return it->second;
    }

    //------------------------------------------------------------
    //! 1文字分ペンを進めます。
    //------------------------------------------------------------
    bool DynamicFontAtlas::AdvancePen(char32_t ch, float& penX, float& penY, float& maxPenX) {
        if(ch == U'\n') {
            if(penX > maxPenX)
                maxPenX = penX;
            penX = 0.0f;
            penY += m_lineHeight;
            return false;
        }

        penX += GetGlyphMetrics(ch).advanceX;
        if(penX > maxPenX)
            maxPenX = penX;
        return true;
    }

    //------------------------------------------------------------
    //! @brief  文字列の描画サイズを取得する関数
    //------------------------------------------------------------
    hlslpp::float2 DynamicFontAtlas::MeasureString(std::string_view text) {
        if(!m_font || text.empty())
            return hlslpp::float2(0.0f, 0.0f);

        float penX    = 0.0f;
        float penY    = 0.0f;
        float maxPenX = 0.0f;

        for(size_t index = 0; index < text.size();) {
            AdvancePen(Tsukino::Core::Utf8::DecodeNext(text, index), penX, penY, maxPenX);
        }

        // penYは「最終行の先頭までの送り量」なので、1行分の高さを足したものが全体の高さになる
        return hlslpp::float2(maxPenX, penY + m_lineHeight);
    }

    //------------------------------------------------------------
    //! @brief  文字列を、グリフ1つにつき1枚のスプライトにして並びへ足す関数
    //------------------------------------------------------------
    void DynamicFontAtlas::AppendStringSprites(std::vector<SpriteQuad>& outSprites, std::string_view text,
                                               hlslpp::float2 position, hlslpp::float4 color, hlslpp::float2 origin, float scale,
                                               hlslpp::float4 outlineColor, float outlineWidth, float rotation) {
        if(!m_font || scale <= 0.0f)
            return;

        // 回転。グリフの左上の位置を position を軸に回し、グリフ自体も同じ角度で（左上を軸に）回す
        const bool  rotated = rotation != 0.0f;
        const float cosR    = std::cos(rotation);
        const float sinR    = std::sin(rotation);

        //------------------------------------------------------------
        // 描く大きさ以上で最も近い段でラスタライズしたグリフを使い、
        // 段との差だけを描画時の縮小率で埋める（段の理由はヘッダの kRasterSizeStep 参照）
        //------------------------------------------------------------
        const float drawSize    = m_pixelSize * scale;
        const int   rasterSize  = ToRasterSize(drawSize);
        const float rasterScale = drawSize / static_cast<float>(rasterSize);

        //------------------------------------------------------------
        // 1パス分のスプライト。offsetX/offsetYだけずらした位置に指定色で文字列を並べる
        //------------------------------------------------------------
        auto drawPass = [&](float offsetX, float offsetY, hlslpp::float4 passColor) {
            const float passR = passColor.x;
            const float passG = passColor.y;
            const float passB = passColor.z;
            const float passA = passColor.w;

            const float startX = position.x - origin.x * scale + offsetX;
            const float startY = position.y - origin.y * scale + offsetY;

            // position は上端(top-left)を指す運用にしたいため、最初の行のベースラインを
            // アセント分だけ下げる(グリフの座標はベースライン基準のため)
            float penX    = 0.0f;
            float penY    = m_ascent;
            float maxPenX = 0.0f;

            for(size_t index = 0; index < text.size();) {
                const char32_t ch = Tsukino::Core::Utf8::DecodeNext(text, index);

                // 送る前のペン位置がこのグリフの描画基準になる
                const float glyphPenX = penX;
                const float glyphPenY = penY;

                if(!AdvancePen(ch, penX, penY, maxPenX))
                    continue;    // 改行

                const GlyphImage& glyph = GetOrRasterizeGlyph(ch, rasterSize);
                if(!glyph.hasInk)
                    continue;    // 実体のないグリフ（半角/全角スペース等）

                const float glyphX = startX + glyphPenX * scale + glyph.bearingX * rasterScale;
                const float glyphY = startY + glyphPenY * scale + glyph.bearingY * rasterScale;

                SpriteQuad& sprite = outSprites.emplace_back();
                if(rotated) {
                    // 回したときは整数ピクセルにそろえない（そろえると文字がガタガタに並ぶ）
                    const float dx = glyphX - position.x;
                    const float dy = glyphY - position.y;
                    sprite.x       = position.x + dx * cosR - dy * sinR;
                    sprite.y       = position.y + dx * sinR + dy * cosR;
                } else {
                    // グリフの左上を整数ピクセルにそろえる（半端な位置だとバイリニアサンプリングでにじむ）
                    sprite.x = std::round(glyphX);
                    sprite.y = std::round(glyphY);
                }

                sprite.texture  = m_pages[glyph.page].texture;
                sprite.source   = {glyph.atlasRect.left, glyph.atlasRect.top, glyph.atlasRect.right, glyph.atlasRect.bottom};
                sprite.color[0] = passR;
                sprite.color[1] = passG;
                sprite.color[2] = passB;
                sprite.color[3] = passA;
                sprite.rotation = rotation;
                sprite.scaleX   = rasterScale;
                sprite.scaleY   = rasterScale;
            }
        };

        //------------------------------------------------------------
        // 縁取り：本体より先に周りへずらして並べる。スプライトは並べた順に
        // 描かれるので、縁取りが本体の下に入る。
        // ずらす向きは円周上に等間隔に取り、縁を丸くする（太いときは向きを増やして、でこぼこを減らす）
        //------------------------------------------------------------
        if(outlineWidth > 0.0f && outlineColor.w > 0.0f) {
            const int directions = (outlineWidth >= 2.0f) ? 16 : 8;
            for(int i = 0; i < directions; ++i) {
                const float angle = 6.28318530718f * static_cast<float>(i) / static_cast<float>(directions);
                drawPass(std::cos(angle) * outlineWidth, std::sin(angle) * outlineWidth, outlineColor);
            }
        }

        drawPass(0.0f, 0.0f, color);
    }

}    // namespace Tsukino::Renderer

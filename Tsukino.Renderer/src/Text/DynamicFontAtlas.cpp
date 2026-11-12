//------------------------------------------------------------
//! @file	DynamicFontAtlas.cpp
//! @brief	DirectWriteによるオンデマンドグリフラスタライズと動的テクスチャアトラスの実装
//! @author 山﨑愛
//------------------------------------------------------------
#include <Tsukino/Renderer/Text/DynamicFontAtlas.hpp>
#include <Tsukino/Renderer/RenderResources.hpp>

#include <Tsukino/Core/Log.hpp>

#include <Tsukino/Core/WindowsLean.hpp>
#include <dwrite_3.h>
#include <wrl/client.h>

#include <algorithm>
#include <cmath>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    using Microsoft::WRL::ComPtr;

    //------------------------------------------------------------
    //! フォントの読み込みとラスタライズに使う DirectWrite のオブジェクトです。
    //------------------------------------------------------------
    struct DynamicFontAtlas::DirectWrite {
        ComPtr<IDWriteFactory5>               factory;           // DirectWrite のファクトリ
        ComPtr<IDWriteInMemoryFontFileLoader> fontFileLoader;    // メモリ上のフォントを読むローダー
        ComPtr<IDWriteFontFace>               fontFace;          // 読み込んだフォント
    };

    //------------------------------------------------------------
    //! @brief  コンストラクタ
    //------------------------------------------------------------
    DynamicFontAtlas::DynamicFontAtlas(RenderResources& resources, const uint8_t* fontData, size_t fontDataSize, float pixelSize)
        : m_resources(&resources), m_dwrite(std::make_unique<DirectWrite>()), m_pixelSize(pixelSize) {
        //--------------------------------------------------------------
        // DirectWriteファクトリの作成
        //--------------------------------------------------------------
        HRESULT hr = ::DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory5), reinterpret_cast<IUnknown**>(m_dwrite->factory.GetAddressOf()));
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to create IDWriteFactory5.");
            return;
        }

        //--------------------------------------------------------------
        // メモリ上のフォントデータをロードするためのローダーを登録
        //--------------------------------------------------------------
        hr = m_dwrite->factory->CreateInMemoryFontFileLoader(m_dwrite->fontFileLoader.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to create IDWriteInMemoryFontFileLoader.");
            return;
        }

        hr = m_dwrite->factory->RegisterFontFileLoader(m_dwrite->fontFileLoader.Get());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to register font file loader.");
            return;
        }

        ComPtr<IDWriteFontFile> fontFile;
        hr = m_dwrite->fontFileLoader->CreateInMemoryFontFileReference(m_dwrite->factory.Get(), fontData, static_cast<UINT32>(fontDataSize), nullptr, &fontFile);
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to create in-memory font file reference.");
            return;
        }

        //--------------------------------------------------------------
        // フォントフォーマット(TrueType/CFF等)を解析してからフォントフェイスを作成
        //--------------------------------------------------------------
        BOOL                   isSupported = FALSE;
        DWRITE_FONT_FILE_TYPE  fileType    = DWRITE_FONT_FILE_TYPE_UNKNOWN;
        DWRITE_FONT_FACE_TYPE  faceType    = DWRITE_FONT_FACE_TYPE_UNKNOWN;
        UINT32                 numFaces    = 0;

        hr = fontFile->Analyze(&isSupported, &fileType, &faceType, &numFaces);
        if(FAILED(hr) || !isSupported || numFaces == 0) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Unsupported font file.");
            return;
        }

        IDWriteFontFile* fontFileArray[] = {fontFile.Get()};
        hr = m_dwrite->factory->CreateFontFace(faceType, 1, fontFileArray, 0, DWRITE_FONT_SIMULATIONS_NONE, &m_dwrite->fontFace);
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to create font face.");
            return;
        }

        //--------------------------------------------------------------
        // メトリクスからピクセル換算スケールと行送り量を計算
        //--------------------------------------------------------------
        DWRITE_FONT_METRICS metrics{};
        m_dwrite->fontFace->GetMetrics(&metrics);

        m_scale      = pixelSize / static_cast<float>(metrics.designUnitsPerEm);
        m_lineHeight = static_cast<float>(metrics.ascent + metrics.descent + metrics.lineGap) * m_scale;
        m_ascent     = static_cast<float>(metrics.ascent) * m_scale;
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
    const DynamicFontAtlas::GlyphMetrics& DynamicFontAtlas::GetGlyphMetrics(wchar_t codepoint) {
        const uint32_t key = static_cast<uint32_t>(codepoint);
        if(auto it = m_metricsCache.find(key); it != m_metricsCache.end())
            return it->second;

        GlyphMetrics metrics{};
        UINT32       codepoint32 = key;
        m_dwrite->fontFace->GetGlyphIndices(&codepoint32, 1, &metrics.glyphIndex);

        // 文字送りはフォントの設計値から求める（描く大きさによらず、位置がずれない）
        DWRITE_GLYPH_METRICS designMetrics{};
        m_dwrite->fontFace->GetDesignGlyphMetrics(&metrics.glyphIndex, 1, &designMetrics, FALSE);
        metrics.advanceX = static_cast<float>(designMetrics.advanceWidth) * m_scale;

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
    const DynamicFontAtlas::GlyphImage& DynamicFontAtlas::GetOrRasterizeGlyph(wchar_t codepoint, int rasterSize) {
        const uint64_t key = (static_cast<uint64_t>(rasterSize) << 32) | static_cast<uint32_t>(codepoint);
        if(auto it = m_glyphCache.find(key); it != m_glyphCache.end())
            return it->second;

        GlyphImage          image{};
        const GlyphMetrics& metrics = GetGlyphMetrics(codepoint);

        DWRITE_GLYPH_RUN run{};
        run.fontFace     = m_dwrite->fontFace.Get();
        run.fontEmSize   = static_cast<float>(rasterSize);
        run.glyphCount   = 1;
        run.glyphIndices = &metrics.glyphIndex;

        //--------------------------------------------------------------
        // グレースケールのアンチエイリアスでラスタライズする。
        // ALIASED（白黒の2値）だと縁が段々になり、縮小・拡大するとさらに目立つ。
        // グリッドフィットを切るのは、大きさごとに字形が崩れて見えるのを防ぐため
        //--------------------------------------------------------------
        ComPtr<IDWriteGlyphRunAnalysis> analysis;
        IDWriteFactory3*                factory3 = m_dwrite->factory.Get();
        HRESULT hr = factory3->CreateGlyphRunAnalysis(&run, nullptr, DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC, DWRITE_MEASURING_MODE_NATURAL,
                                                      DWRITE_GRID_FIT_MODE_DISABLED, DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE, 0.0f, 0.0f, &analysis);
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to create glyph run analysis.");
            auto [it, _] = m_glyphCache.emplace(key, image);
            return it->second;
        }

        //--------------------------------------------------------------
        // カバレッジを取り出す。グレースケールの結果は 1x1（1ピクセル1バイト）で返る環境と、
        // 3x1（RGB の3バイト。グレースケールなので3つとも同じ値）で返る環境があるので両方に対応する
        //--------------------------------------------------------------
        RECT                 bounds{};
        DWRITE_TEXTURE_TYPE  textureType  = DWRITE_TEXTURE_ALIASED_1x1;
        uint32_t             bytesPerTexel = 1;
        analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1, &bounds);
        if(bounds.right <= bounds.left || bounds.bottom <= bounds.top) {
            textureType   = DWRITE_TEXTURE_CLEARTYPE_3x1;
            bytesPerTexel = 3;
            analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_CLEARTYPE_3x1, &bounds);
        }

        const uint32_t width  = static_cast<uint32_t>(std::max<LONG>(0, bounds.right - bounds.left));
        const uint32_t height = static_cast<uint32_t>(std::max<LONG>(0, bounds.bottom - bounds.top));

        // インクの無いグリフ(全角スペース・結合文字等)は位置情報だけキャッシュする
        if(width == 0 || height == 0) {
            auto [it, _] = m_glyphCache.emplace(key, image);
            return it->second;
        }

        std::vector<uint8_t> coverage(static_cast<size_t>(width) * height * bytesPerTexel);
        hr = analysis->CreateAlphaTexture(textureType, &bounds, coverage.data(), static_cast<UINT32>(coverage.size()));
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to get alpha texture.");
            auto [it, _] = m_glyphCache.emplace(key, image);
            return it->second;
        }

        // 白RGB + カバレッジ値のアルファへ展開する
        // (SpriteBatchの既定シェーダーはRGBA全チャンネルをサンプルして頂点色を乗算するため、
        //  単チャンネルフォーマットのままではカラーが正しく乗らない)
        std::vector<uint8_t> rgbaBuffer(static_cast<size_t>(width) * height * 4);
        for(size_t i = 0; i < static_cast<size_t>(width) * height; ++i) {
            rgbaBuffer[i * 4 + 0] = 255;
            rgbaBuffer[i * 4 + 1] = 255;
            rgbaBuffer[i * 4 + 2] = 255;
            rgbaBuffer[i * 4 + 3] = coverage[i * bytesPerTexel];
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
        image.bearingX  = static_cast<float>(bounds.left);
        image.bearingY  = static_cast<float>(bounds.top);

        auto [it, _] = m_glyphCache.emplace(key, image);
        return it->second;
    }

    //------------------------------------------------------------
    //! 1文字分ペンを進めます。
    //------------------------------------------------------------
    bool DynamicFontAtlas::AdvancePen(wchar_t ch, float& penX, float& penY, float& maxPenX) {
        if(ch == L'\n') {
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
    hlslpp::float2 DynamicFontAtlas::MeasureString(const std::wstring& text) {
        if(!m_dwrite->fontFace || text.empty())
            return hlslpp::float2(0.0f, 0.0f);

        float penX    = 0.0f;
        float penY    = 0.0f;
        float maxPenX = 0.0f;

        for(wchar_t ch : text) {
            AdvancePen(ch, penX, penY, maxPenX);
        }

        // penYは「最終行の先頭までの送り量」なので、1行分の高さを足したものが全体の高さになる
        return hlslpp::float2(maxPenX, penY + m_lineHeight);
    }

    //------------------------------------------------------------
    //! @brief  文字列を、グリフ1つにつき1枚のスプライトにして並びへ足す関数
    //------------------------------------------------------------
    void DynamicFontAtlas::AppendStringSprites(std::vector<SpriteQuad>& outSprites, const std::wstring& text,
                                               hlslpp::float2 position, hlslpp::float4 color, hlslpp::float2 origin, float scale,
                                               hlslpp::float4 outlineColor, float outlineWidth, float rotation) {
        if(!m_dwrite->fontFace || scale <= 0.0f)
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
            // アセント分だけ下げる(DirectWriteのグリフ座標はベースライン基準のため)
            float penX    = 0.0f;
            float penY    = m_ascent;
            float maxPenX = 0.0f;

            for(wchar_t ch : text) {
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

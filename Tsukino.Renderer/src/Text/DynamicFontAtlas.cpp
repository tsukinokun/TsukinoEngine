//------------------------------------------------------------
//! @file	DynamicFontAtlas.cpp
//! @brief	DirectWriteによるオンデマンドグリフラスタライズと動的テクスチャアトラスの実装
//! @author 山﨑愛
//------------------------------------------------------------
#include <Tsukino/Renderer/Text/DynamicFontAtlas.hpp>

#include <Tsukino/Core/Log.hpp>

#include <algorithm>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------
    //! @brief  コンストラクタ
    //------------------------------------------------------------
    DynamicFontAtlas::DynamicFontAtlas(ID3D11Device* device, const uint8_t* fontData, size_t fontDataSize, float pixelSize)
        : m_device(device), m_pixelSize(pixelSize) {
        //--------------------------------------------------------------
        // DirectWriteファクトリの作成
        //--------------------------------------------------------------
        HRESULT hr = ::DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory5), reinterpret_cast<IUnknown**>(m_factory.GetAddressOf()));
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to create IDWriteFactory5.");
            return;
        }

        //--------------------------------------------------------------
        // メモリ上のフォントデータをロードするためのローダーを登録
        //--------------------------------------------------------------
        hr = m_factory->CreateInMemoryFontFileLoader(m_fontFileLoader.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to create IDWriteInMemoryFontFileLoader.");
            return;
        }

        hr = m_factory->RegisterFontFileLoader(m_fontFileLoader.Get());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to register font file loader.");
            return;
        }

        ComPtr<IDWriteFontFile> fontFile;
        hr = m_fontFileLoader->CreateInMemoryFontFileReference(m_factory.Get(), fontData, static_cast<UINT32>(fontDataSize), nullptr, &fontFile);
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
        hr = m_factory->CreateFontFace(faceType, 1, fontFileArray, 0, DWRITE_FONT_SIMULATIONS_NONE, &m_fontFace);
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to create font face.");
            return;
        }

        //--------------------------------------------------------------
        // メトリクスからピクセル換算スケールと行送り量を計算
        //--------------------------------------------------------------
        DWRITE_FONT_METRICS metrics{};
        m_fontFace->GetMetrics(&metrics);

        m_scale      = pixelSize / static_cast<float>(metrics.designUnitsPerEm);
        m_lineHeight = static_cast<float>(metrics.ascent + metrics.descent + metrics.lineGap) * m_scale;
        m_ascent     = static_cast<float>(metrics.ascent) * m_scale;
    }

    //------------------------------------------------------------
    //! @brief  新しいアトラスページを作成する関数
    //------------------------------------------------------------
    bool DynamicFontAtlas::CreatePage() {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width            = kPageSize;
        desc.Height           = kPageSize;
        desc.MipLevels        = 1;
        desc.ArraySize        = 1;
        desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage            = D3D11_USAGE_DEFAULT;
        desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

        Page page;
        HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, page.texture.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to create atlas page texture.");
            return false;
        }

        hr = m_device->CreateShaderResourceView(page.texture.Get(), nullptr, page.srv.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to create atlas page SRV.");
            return false;
        }

        m_pages.push_back(std::move(page));
        return true;
    }

    //------------------------------------------------------------
    //! @brief  指定サイズの矩形をアトラスに確保する関数
    //------------------------------------------------------------
    bool DynamicFontAtlas::AllocateRect(uint32_t width, uint32_t height, int& outPage, uint32_t& outX, uint32_t& outY) {
        if(width > kPageSize || height > kPageSize) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Glyph too large for atlas page.");
            return false;
        }

        if(m_pages.empty() && !CreatePage())
            return false;

        Page* page = &m_pages.back();

        // 現在のシェルフに収まらなければ次のシェルフへ折り返す
        if(page->cursorX + width > kPageSize) {
            page->cursorX     = 0;
            page->cursorY += page->shelfHeight;
            page->shelfHeight = 0;
        }

        // 現在のページに収まらなければ新しいページを作成する
        if(page->cursorY + height > kPageSize) {
            if(!CreatePage())
                return false;
            page = &m_pages.back();
        }

        outPage = static_cast<int>(m_pages.size() - 1);
        outX    = page->cursorX;
        outY    = page->cursorY;

        page->cursorX += width;
        page->shelfHeight = std::max(page->shelfHeight, height);

        return true;
    }

    //------------------------------------------------------------
    //! @brief  グリフをキャッシュから取得、無ければラスタライズしてキャッシュする関数
    //------------------------------------------------------------
    const DynamicFontAtlas::GlyphInfo& DynamicFontAtlas::GetOrRasterizeGlyph(wchar_t codepoint, ID3D11DeviceContext* context) {
        const uint32_t key = static_cast<uint32_t>(codepoint);

        if(auto it = m_glyphCache.find(key); it != m_glyphCache.end())
            return it->second;

        GlyphInfo info{};

        UINT32 codepoint32 = key;
        UINT16 glyphIndex   = 0;
        m_fontFace->GetGlyphIndices(&codepoint32, 1, &glyphIndex);

        DWRITE_GLYPH_METRICS designMetrics{};
        m_fontFace->GetDesignGlyphMetrics(&glyphIndex, 1, &designMetrics, FALSE);
        info.advanceX = static_cast<float>(designMetrics.advanceWidth) * m_scale;

        DWRITE_GLYPH_RUN run{};
        run.fontFace      = m_fontFace.Get();
        run.fontEmSize    = m_pixelSize;
        run.glyphCount    = 1;
        run.glyphIndices  = &glyphIndex;

        // DWRITE_TEXTURE_ALIASED_1x1(1ピクセル1バイトのグレースケールカバレッジ)を取得するには
        // レンダリングモードもDWRITE_RENDERING_MODE_ALIASEDに合わせる必要がある
        // (NATURAL等のClearType系モードと組み合わせるとGetAlphaTextureBoundsが常に空矩形を返す)
        ComPtr<IDWriteGlyphRunAnalysis> analysis;
        HRESULT hr = m_factory->CreateGlyphRunAnalysis(
            &run, 1.0f, nullptr, DWRITE_RENDERING_MODE_ALIASED, DWRITE_MEASURING_MODE_NATURAL, 0.0f, 0.0f, &analysis);
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to create glyph run analysis.");
            auto [it, _] = m_glyphCache.emplace(key, info);
            return it->second;
        }

        RECT bounds{};
        analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1, &bounds);

        const uint32_t width  = static_cast<uint32_t>(bounds.right - bounds.left);
        const uint32_t height = static_cast<uint32_t>(bounds.bottom - bounds.top);

        // インクの無いグリフ(全角スペース・結合文字等)はアドバンス情報のみキャッシュする
        if(width == 0 || height == 0) {
            auto [it, _] = m_glyphCache.emplace(key, info);
            return it->second;
        }

        std::vector<uint8_t> alphaBuffer(static_cast<size_t>(width) * height);
        hr = analysis->CreateAlphaTexture(DWRITE_TEXTURE_ALIASED_1x1, &bounds, alphaBuffer.data(), static_cast<UINT32>(alphaBuffer.size()));
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("DynamicFontAtlas: Failed to get alpha texture.");
            auto [it, _] = m_glyphCache.emplace(key, info);
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
            rgbaBuffer[i * 4 + 3] = alphaBuffer[i];
        }

        int      pageIndex = 0;
        uint32_t atlasX = 0, atlasY = 0;
        if(!AllocateRect(width, height, pageIndex, atlasX, atlasY)) {
            auto [it, _] = m_glyphCache.emplace(key, info);
            return it->second;
        }

        D3D11_BOX box{};
        box.left   = atlasX;
        box.top    = atlasY;
        box.front  = 0;
        box.right  = atlasX + width;
        box.bottom = atlasY + height;
        box.back   = 1;

        context->UpdateSubresource(m_pages[pageIndex].texture.Get(), 0, &box, rgbaBuffer.data(), width * 4, 0);

        info.hasInk   = true;
        info.page     = pageIndex;
        info.atlasRect = RECT{static_cast<LONG>(atlasX), static_cast<LONG>(atlasY), static_cast<LONG>(atlasX + width), static_cast<LONG>(atlasY + height)};
        info.bearingX = static_cast<float>(bounds.left);
        info.bearingY = static_cast<float>(bounds.top);

        auto [it, _] = m_glyphCache.emplace(key, info);
        return it->second;
    }

    //------------------------------------------------------------
    //! @brief  1文字分ペンを進める関数
    //------------------------------------------------------------
    const DynamicFontAtlas::GlyphInfo* DynamicFontAtlas::AdvancePen(wchar_t ch, ID3D11DeviceContext* context, float& penX, float& penY,
                                                                     float& maxPenX) {
        if(ch == L'\n') {
            if(penX > maxPenX)
                maxPenX = penX;
            penX = 0.0f;
            penY += m_lineHeight;
            return nullptr;
        }

        const GlyphInfo& glyph = GetOrRasterizeGlyph(ch, context);

        penX += glyph.advanceX;
        if(penX > maxPenX)
            maxPenX = penX;

        return &glyph;
    }

    //------------------------------------------------------------
    //! @brief  文字列の描画サイズを取得する関数
    //------------------------------------------------------------
    hlslpp::float2 DynamicFontAtlas::MeasureString(const std::wstring& text, ID3D11DeviceContext* context) {
        if(!m_fontFace || text.empty())
            return hlslpp::float2(0.0f, 0.0f);

        float penX    = 0.0f;
        float penY    = 0.0f;
        float maxPenX = 0.0f;

        for(wchar_t ch : text) {
            AdvancePen(ch, context, penX, penY, maxPenX);
        }

        // penYは「最終行の先頭までの送り量」なので、1行分の高さを足したものが全体の高さになる
        return hlslpp::float2(maxPenX, penY + m_lineHeight);
    }

    //------------------------------------------------------------
    //! @brief  文字列を描画する関数
    //------------------------------------------------------------
    void DynamicFontAtlas::DrawString(DirectX::SpriteBatch* spriteBatch, ID3D11DeviceContext* context, const std::wstring& text,
                                       hlslpp::float2 position, hlslpp::float4 color, hlslpp::float2 origin, float scale,
                                       hlslpp::float4 outlineColor, float outlineWidth) {
        if(!m_fontFace)
            return;

        //------------------------------------------------------------
        // 1パス分の描画。offsetX/offsetYだけずらした位置に指定色で文字列を描く
        //------------------------------------------------------------
        auto drawPass = [&](float offsetX, float offsetY, hlslpp::float4 passColor) {
            DirectX::XMFLOAT4 dxColor(passColor.x, passColor.y, passColor.z, passColor.w);
            DirectX::XMVECTOR colorVec = DirectX::XMLoadFloat4(&dxColor);

            const DirectX::XMFLOAT2 penStart(position.x - origin.x * scale + offsetX, position.y - origin.y * scale + offsetY);

            // position は上端(top-left)を指す運用にしたいため、最初の行のベースラインを
            // アセント分だけ下げる(DirectWriteのグリフ座標はベースライン基準のため)
            float penX    = 0.0f;
            float penY    = m_ascent;
            float maxPenX = 0.0f;

            for(wchar_t ch : text) {
                // 送る前のペン位置がこのグリフの描画基準になる
                const float glyphPenX = penX;
                const float glyphPenY = penY;

                const GlyphInfo* glyph = AdvancePen(ch, context, penX, penY, maxPenX);
                if(!glyph || !glyph->hasInk)
                    continue;    // 改行、または実体のないグリフ（半角/全角スペース等）

                RECT sourceRect = glyph->atlasRect;

                DirectX::XMFLOAT2 destPos(penStart.x + (glyphPenX + glyph->bearingX) * scale,
                                          penStart.y + (glyphPenY + glyph->bearingY) * scale);

                spriteBatch->Draw(m_pages[glyph->page].srv.Get(), destPos, &sourceRect, colorVec, 0.0f, DirectX::XMFLOAT2(0.0f, 0.0f), scale);
            }
        };

        //------------------------------------------------------------
        // 縁取り：本体より先に8方向へずらして描く。SpriteSortMode_Deferredなので
        // 積んだ順にそのまま描かれ、縁取りが本体の下に入る
        //------------------------------------------------------------
        if(outlineWidth > 0.0f && outlineColor.w > 0.0f) {
            constexpr float kOutlineOffsets[8][2] = {
                {-1.0f, -1.0f}, {0.0f, -1.0f}, {1.0f, -1.0f},
                {-1.0f,  0.0f},                {1.0f,  0.0f},
                {-1.0f,  1.0f}, {0.0f,  1.0f}, {1.0f,  1.0f},
            };

            for(const auto& offset : kOutlineOffsets) {
                drawPass(offset[0] * outlineWidth, offset[1] * outlineWidth, outlineColor);
            }
        }

        drawPass(0.0f, 0.0f, color);
    }

}    // namespace Tsukino::Renderer

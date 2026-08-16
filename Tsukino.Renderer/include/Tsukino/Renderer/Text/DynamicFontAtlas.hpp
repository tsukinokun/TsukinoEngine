//------------------------------------------------------------
//! @file	DynamicFontAtlas.hpp
//! @brief	DirectWriteによるオンデマンドグリフラスタライズと動的テクスチャアトラスの宣言
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <wrl/client.h>
#include <d3d11.h>
#include <dwrite_3.h>
#include <SpriteBatch.h>

#include <hlsl++.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

using Microsoft::WRL::ComPtr;

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------
    //! @class  DynamicFontAtlas
    //! @brief  グリフを実行時にオンデマンドでラスタライズし、GPUテクスチャアトラスへキャッシュするクラス
    //! @note   事前ベイクを行わないため、日本語のような文字種が膨大な言語でも
    //!         実際に描画で使われた文字分のコストしかかからない
    //------------------------------------------------------------
    class DynamicFontAtlas {
    public:
        //------------------------------------------------------------
        //! @brief  コンストラクタ
        //! @param  device        [in] D3D11デバイス
        //! @param  fontData      [in] ttf/otfの生データ
        //! @param  fontDataSize  [in] fontDataのバイト数
        //! @param  pixelSize     [in] グリフをラスタライズする基準ピクセルサイズ
        //------------------------------------------------------------
        DynamicFontAtlas(ID3D11Device* device, const uint8_t* fontData, size_t fontDataSize, float pixelSize);

        // コピー禁止（GPUリソースを保持するため）
        DynamicFontAtlas(const DynamicFontAtlas&)            = delete;
        DynamicFontAtlas& operator=(const DynamicFontAtlas&) = delete;

        //------------------------------------------------------------
        //! @brief  文字列を描画する関数
        //! @param  spriteBatch [in] 描画に使用するSpriteBatch（Begin～Endの間で呼び出すこと）
        //! @param  context     [in] D3D11デバイスコンテキスト（未キャッシュのグリフのラスタライズに使用）
        //! @param  text        [in] 描画するテキスト
        //! @param  position    [in] 描画位置
        //! @param  color       [in] 文字色
        //! @param  origin      [in] 原点（左上からのオフセット、微調整用）
        //! @param  scale       [in] 追加スケール（Transformのワールドスケールなど）
        //------------------------------------------------------------
        void DrawString(DirectX::SpriteBatch* spriteBatch, ID3D11DeviceContext* context, const std::wstring& text, hlslpp::float2 position,
                         hlslpp::float4 color, hlslpp::float2 origin, float scale);

    private:
        //------------------------------------------------------------
        //! @struct GlyphInfo
        //! @brief  1グリフ分のアトラス上の位置・メトリクス情報
        //------------------------------------------------------------
        struct GlyphInfo {
            RECT  atlasRect{};      // アトラスページ内の矩形（ピクセル座標）
            int   page      = 0;    // 所属ページのインデックス
            float bearingX  = 0.0f; // ペン位置からのインクの左オフセット
            float bearingY  = 0.0f; // ベースラインからのインクの上オフセット
            float advanceX  = 0.0f; // 次の文字へのペン送り量
            bool  hasInk    = false;// 実体のあるグリフか（全角スペース等はfalse）
        };

        //------------------------------------------------------------
        //! @struct Page
        //! @brief  アトラスの1ページ分のGPUリソースとシェルフパッカーの状態
        //------------------------------------------------------------
        struct Page {
            ComPtr<ID3D11Texture2D>          texture;
            ComPtr<ID3D11ShaderResourceView> srv;
            uint32_t                         cursorX     = 0;    // 現在のシェルフ内での書き込みX位置
            uint32_t                         cursorY     = 0;    // 現在のシェルフのY位置
            uint32_t                         shelfHeight = 0;    // 現在のシェルフの高さ
        };

        static constexpr uint32_t kPageSize = 1024;    // 1ページあたりの一辺のピクセル数

        //------------------------------------------------------------
        //! @brief  グリフをキャッシュから取得、無ければラスタライズしてキャッシュする関数
        //------------------------------------------------------------
        const GlyphInfo& GetOrRasterizeGlyph(wchar_t codepoint, ID3D11DeviceContext* context);

        //------------------------------------------------------------
        //! @brief  新しいアトラスページを作成する関数
        //------------------------------------------------------------
        bool CreatePage();

        //------------------------------------------------------------
        //! @brief  指定サイズの矩形をアトラスに確保する関数（必要なら新しいページを作成する）
        //! @return 確保できた場合 true
        //------------------------------------------------------------
        bool AllocateRect(uint32_t width, uint32_t height, int& outPage, uint32_t& outX, uint32_t& outY);

        ID3D11Device* m_device = nullptr;    // Rendererが所有するデバイス（非所有の参照）

        ComPtr<IDWriteFactory5>                 m_factory;
        ComPtr<IDWriteInMemoryFontFileLoader>    m_fontFileLoader;
        ComPtr<IDWriteFontFace>                  m_fontFace;

        float m_pixelSize  = 0.0f;    // グリフをラスタライズする基準ピクセルサイズ
        float m_scale      = 1.0f;    // フォントデザイン単位 → ピクセルへの変換係数
        float m_lineHeight = 0.0f;    // 改行時の行送り量（ピクセル、m_pixelSize基準）
        float m_ascent     = 0.0f;    // ベースラインから上端までの距離（ピクセル、m_pixelSize基準）

        std::vector<Page>                          m_pages;
        std::unordered_map<uint32_t, GlyphInfo>    m_glyphCache;
    };

}    // namespace Tsukino::Renderer

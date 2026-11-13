//----------------------------------------------------------------------------
//! @file   BakedFont.cpp
//! @brief  事前ベイクしたフォント（.spritefont）の実装（DX11 版。DirectXTK の SpriteFont を使う）
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/Text/BakedFont.hpp>
#include <Tsukino/Renderer/DX11/RenderResourcesGpu.hpp>

#include <SpriteFont.h>
#include <wrl/client.h>

#include <cwctype>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! BakedFont の中身です。
    //------------------------------------------------------------------------
    struct BakedFont::Impl {
        std::unique_ptr<DirectX::SpriteFont> font;     // DirectXTK のフォント
        TextureHandle                        sheet;    // フォントの文字の画像を指すハンドル
    };

    //------------------------------------------------------------------------
    //! フォントのバイナリを読み込みます。
    //------------------------------------------------------------------------
    std::unique_ptr<BakedFont> BakedFont::Create(RenderResources& resources, const u8* data, std::size_t size) {
        auto impl  = std::make_unique<Impl>();
        impl->font = resources.GetGpu().CreateSpriteFont(data, size);
        if(!impl->font)
            return nullptr;

        // 文字の画像はフォントが持っている。描くときにハンドルで指せるように登録しておく
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sheet;
        impl->font->GetSpriteSheet(sheet.GetAddressOf());
        impl->sheet = resources.GetGpu().RegisterExternalTexture(sheet.Get());

        return std::unique_ptr<BakedFont>(new BakedFont(std::move(impl)));
    }

    //------------------------------------------------------------------------
    //! 中身を受け取って作ります。
    //------------------------------------------------------------------------
    BakedFont::BakedFont(std::unique_ptr<Impl> impl)
        : m_impl(std::move(impl)) {}

    //------------------------------------------------------------------------
    //! フォントを破棄します。
    //------------------------------------------------------------------------
    BakedFont::~BakedFont() = default;

    //------------------------------------------------------------------------
    //! 文字をフォントが持っているかを返します。
    //------------------------------------------------------------------------
    bool BakedFont::ContainsCharacter(wchar_t ch) const {
        return m_impl->font->ContainsCharacter(ch);
    }

    //------------------------------------------------------------------------
    //! 文字列の描画サイズを測ります。
    //------------------------------------------------------------------------
    hlslpp::float2 BakedFont::MeasureString(const std::wstring& text) const {
        DirectX::XMFLOAT2 size{};
        DirectX::XMStoreFloat2(&size, m_impl->font->MeasureString(text.c_str()));
        return hlslpp::float2(size.x, size.y);
    }

    //------------------------------------------------------------------------
    //! 文字列を、グリフ1つにつき1枚のスプライトにして並びへ足します。
    //------------------------------------------------------------------------
    void BakedFont::AppendStringSprites(std::vector<SpriteQuad>& outSprites, const std::wstring& text, hlslpp::float2 position,
                                        hlslpp::float4 color, float rotation, hlslpp::float2 origin, float scale) const {
        if(!m_impl->sheet.IsValid())
            return;

        const float lineSpacing = m_impl->font->GetLineSpacing();
        const float originX     = origin.x;
        const float originY     = origin.y;

        float x = 0.0f;    // 行内のペン位置
        float y = 0.0f;    // 行の上端
        for(wchar_t ch : text) {
            if(ch == L'\r')
                continue;

            if(ch == L'\n') {
                x = 0.0f;
                y += lineSpacing;
                continue;
            }

            const DirectX::SpriteFont::Glyph* glyph = m_impl->font->FindGlyph(ch);

            x += glyph->XOffset;
            if(x < 0.0f)
                x = 0.0f;

            const LONG  width   = glyph->Subrect.right - glyph->Subrect.left;
            const LONG  height  = glyph->Subrect.bottom - glyph->Subrect.top;
            const float advance = static_cast<float>(width) + glyph->XAdvance;

            // 空白は描かない（1ピクセルより大きいインクを持つものだけ描く）
            if(!std::iswspace(ch) || width > 1 || height > 1) {
                SpriteQuad& sprite = outSprites.emplace_back();
                sprite.texture  = m_impl->sheet;
                sprite.x        = position.x;
                sprite.y        = position.y;
                sprite.source   = {static_cast<i32>(glyph->Subrect.left), static_cast<i32>(glyph->Subrect.top),
                                   static_cast<i32>(glyph->Subrect.right), static_cast<i32>(glyph->Subrect.bottom)};
                sprite.color[0] = color.x;
                sprite.color[1] = color.y;
                sprite.color[2] = color.z;
                sprite.color[3] = color.w;
                sprite.rotation = rotation;
                sprite.originX  = originX - x;
                sprite.originY  = originY - (y + glyph->YOffset);
                sprite.scaleX   = scale;
                sprite.scaleY   = scale;
            }

            x += advance;
        }
    }
}    // namespace Tsukino::Renderer

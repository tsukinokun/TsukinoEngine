//----------------------------------------------------------------------------
//! @file   EffectRenderer.hpp
//! @brief  DX11 版の Effekseer を描く部品の持ち主（Renderer の内部用）
//! @detail Effekseer の DX11 版のレンダラーを作って持ちます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>

#include <EffekseerRendererDX11.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! Effekseer を描く部品の持ち主です（DX11 版）。
    //------------------------------------------------------------------------
    class EffectRenderer {
    public:
        //! Effekseer の DX11 版のレンダラーを作ります。
        //! @param  [in] device         デバイス
        //! @param  [in] context        デバイスコンテキスト
        //! @param  [in] squareMaxCount 1回にまとめて描ける四角の数
        //! @return 作れたら true
        [[nodiscard]]
        bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, int32_t squareMaxCount) {
            // エンジンはリバース Z（手前ほど深度が大きい）なので、深度テストを GREATER_EQUAL にする。
            // 既定の LESS_EQUAL のままだと、深度を張ったときに手前の物の後ろだけが描かれて逆になる
            m_renderer = EffekseerRendererDX11::Renderer::Create(device, context, squareMaxCount, D3D11_COMPARISON_GREATER_EQUAL);
            return m_renderer != nullptr;
        }

        //! Effekseer のレンダラーを返します。
        //! @return レンダラー（作れていなければ nullptr）
        [[nodiscard]]
        ::EffekseerRenderer::Renderer* Get() const noexcept {
            return m_renderer.Get();
        }

    private:
        EffekseerRendererDX11::RendererRef m_renderer;    // Effekseer の DX11 版のレンダラー
    };

}    // namespace Tsukino::Renderer

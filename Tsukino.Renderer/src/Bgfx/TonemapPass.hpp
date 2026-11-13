//----------------------------------------------------------------------------
//! @file   TonemapPass.hpp
//! @brief  bgfx 版のトーンマップパス（Renderer の内部用）
//! @detail HDR バッファをトーンマップして、バックバッファへ書きます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

#include <bgfx/bgfx.h>

// 前方宣言
namespace Tsukino::Asset {
    class ShaderAsset;
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class FullscreenPass;    // 前方宣言

    //------------------------------------------------------------------------
    //! トーンマップパスです（bgfx 版）。
    //------------------------------------------------------------------------
    class TonemapPass {
    public:
        //! プログラムとサンプラーを破棄します。
        ~TonemapPass();

        //! プログラムとサンプラーを作成します。
        //! @param  [in] fullscreenPass フルスクリーン三角形の描画（このクラスより長生きする）
        //! @param  [in] ps             トーンマップのピクセルシェーダー（Tonemap.ps。bgfx の形式）
        //! @return true: 作成成功
        [[nodiscard]]
        bool Initialize(const FullscreenPass& fullscreenPass, const Tsukino::Asset::ShaderAsset* ps);

        //! HDR のテクスチャをトーンマップして描きます。
        //! @param  [in] view    描く先のビュー（バックバッファ）
        //! @param  [in] hdr     HDR のテクスチャ
        void Execute(bgfx::ViewId view, bgfx::TextureHandle hdr) const;

    private:
        const FullscreenPass* m_fullscreenPass = nullptr;                  // フルスクリーン三角形（借りている）
        bgfx::ProgramHandle   m_program        = BGFX_INVALID_HANDLE;      // トーンマップのプログラム
        bgfx::UniformHandle   m_hdrSampler     = BGFX_INVALID_HANDLE;      // s_hdr
    };

}    // namespace Tsukino::Renderer

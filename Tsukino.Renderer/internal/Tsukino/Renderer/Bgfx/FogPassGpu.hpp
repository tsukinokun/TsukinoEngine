//----------------------------------------------------------------------------
//! @file   FogPassGpu.hpp
//! @brief  bgfx 版の FogPass の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/ConstantBuffer.hpp>
#include <Tsukino/Renderer/FogPass.hpp>
#include <Tsukino/Renderer/Bgfx/UniformBlock.hpp>

#include <bgfx/bgfx.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! FogPass の中身です。
    //------------------------------------------------------------------------
    struct FogPass::Gpu {
        bgfx::ProgramHandle      program      = BGFX_INVALID_HANDLE;    // フルスクリーン三角形 ＋ Fog.ps
        UniformBlock<CBufferFog> fog;                                   // フォグの定数（u_fog。DX11 版の b8）
        bgfx::UniformHandle      depthSampler = BGFX_INVALID_HANDLE;    // s_gbufferDepth

        //! 半透明のパスの間だけ、モデルとスプライトのシェーダーの中でフォグを掛けさせる定数
        //! （FogPass::BindForwardFog が入れ、UnbindForwardFog が外す。distanceParams.w = 1）
        CBufferFog forwardData{};
        bool       forwardActive = false;    // forwardData を送る区間か

        Gpu() = default;
        Gpu(const Gpu&)            = delete;
        Gpu& operator=(const Gpu&) = delete;

        //! プログラムとユニフォームを破棄します。
        ~Gpu();

        //! 次に Submit するモデル・スプライトの描画へ、フォグの定数を送ります。
        //! @note   bgfx のユニフォームは描画1回ごとの値で、送らなければ前の描画の値が残りうる。
        //!         半透明のパスの外では「何もしない」値（全部 0）を必ず送る
        void ApplyToDraw() const;
    };

}    // namespace Tsukino::Renderer

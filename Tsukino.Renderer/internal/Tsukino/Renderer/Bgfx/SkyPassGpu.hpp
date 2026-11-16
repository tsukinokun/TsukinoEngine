//----------------------------------------------------------------------------
//! @file   SkyPassGpu.hpp
//! @brief  bgfx 版の SkyPass の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/ConstantBuffer.hpp>
#include <Tsukino/Renderer/SkyPass.hpp>
#include <Tsukino/Renderer/Bgfx/UniformBlock.hpp>

#include <bgfx/bgfx.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class FullscreenPass;    // 前方宣言

    //------------------------------------------------------------------------
    //! SkyPass の中身です。
    //! @note  プログラムと定数は、環境光のベイク（IBLBaker）のキャプチャも借りる
    //------------------------------------------------------------------------
    struct SkyPass::Gpu {
        bgfx::ProgramHandle      program = BGFX_INVALID_HANDLE;    // Sky.vs ＋ Sky.ps
        UniformBlock<CBufferSky> sky;                              // 大気散乱の定数（u_sky）
        const FullscreenPass*    fullscreenPass = nullptr;         // フルスクリーン三角形の描画（借りている。Renderer が設定する）
        CBufferSky               data{};                           // このフレームの大気散乱の定数（SkyPass::Execute が覚える。ベイクが送る）

        Gpu()                      = default;
        Gpu(const Gpu&)            = delete;
        Gpu& operator=(const Gpu&) = delete;

        //! プログラムを破棄します。
        ~Gpu();
    };

}    // namespace Tsukino::Renderer

//----------------------------------------------------------------------------
//! @file   MotionBlurPassGpu.hpp
//! @brief  bgfx 版の MotionBlurPass の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/MotionBlurPass.hpp>

#include <bgfx/bgfx.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! MotionBlurPass の中身です。
    //------------------------------------------------------------------------
    struct MotionBlurPass::Gpu {
        bgfx::ProgramHandle program         = BGFX_INVALID_HANDLE;    // フルスクリーン三角形 ＋ MotionBlur.ps
        bgfx::UniformHandle params          = BGFX_INVALID_HANDLE;    // u_motionBlur（x: 強度, y: ブラー長の上限, z: シャッター補正, w: サンプル数）
        bgfx::UniformHandle sceneSampler    = BGFX_INVALID_HANDLE;    // s_motionBlurScene
        bgfx::UniformHandle velocitySampler = BGFX_INVALID_HANDLE;    // s_motionBlurVelocity

        Gpu() = default;
        Gpu(const Gpu&)            = delete;
        Gpu& operator=(const Gpu&) = delete;

        //! プログラムとユニフォームを破棄します。
        ~Gpu();
    };

}    // namespace Tsukino::Renderer

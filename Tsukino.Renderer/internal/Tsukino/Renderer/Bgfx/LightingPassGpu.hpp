//----------------------------------------------------------------------------
//! @file   LightingPassGpu.hpp
//! @brief  bgfx 版の LightingPass の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/ConstantBuffer.hpp>
#include <Tsukino/Renderer/LightingPass.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/Bgfx/UniformBlock.hpp>

#include <bgfx/bgfx.h>

#include <array>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! LightingPass の中身です。
    //------------------------------------------------------------------------
    struct LightingPass::Gpu {
        bgfx::ProgramHandle         program = BGFX_INVALID_HANDLE;    // フルスクリーン三角形 ＋ Lighting.ps
        UniformBlock<CBufferLights> lights;                           // 点光源・スポットライト（u_lights。DX11 版の b5）

        //! G-Buffer と深度のサンプラー（s_gbufferAlbedo・Normal・Material・Emissive・Depth・WorldPos の順）
        std::array<bgfx::UniformHandle, GraphicsContext::kGBufferCount> gbufferSamplers{};

        Gpu() = default;
        Gpu(const Gpu&)            = delete;
        Gpu& operator=(const Gpu&) = delete;

        //! プログラムとサンプラーを破棄します。
        ~Gpu();
    };

}    // namespace Tsukino::Renderer

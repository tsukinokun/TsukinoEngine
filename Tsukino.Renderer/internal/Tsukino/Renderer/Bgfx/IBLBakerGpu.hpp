//----------------------------------------------------------------------------
//! @file   IBLBakerGpu.hpp
//! @brief  bgfx 版の IBLBaker の中身（Renderer の内部用）
//! @detail 環境光（IBL）のテクスチャと定数を持ち、照らすシェーダーの描画へ張ります。
//!         スカイからのベイク（段階 1-4 c）を移すまでは、環境光の無い（黒い）1×1 のテクスチャを張ります。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/IBLBaker.hpp>

#include <bgfx/bgfx.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! IBLBaker の中身です。
    //------------------------------------------------------------------------
    struct IBLBaker::Gpu {
        bgfx::TextureHandle irradiance = BGFX_INVALID_HANDLE;    // 拡散の irradiance（キューブ）
        bgfx::TextureHandle specular   = BGFX_INVALID_HANDLE;    // 鏡面のプレフィルタ（キューブ）
        bgfx::TextureHandle brdfLut    = BGFX_INVALID_HANDLE;    // BRDF の積分表（2D）

        bgfx::UniformHandle iblUniform        = BGFX_INVALID_HANDLE;    // u_ibl（x: スペキュラのミップ数, y: 強さ）
        bgfx::UniformHandle irradianceSampler = BGFX_INVALID_HANDLE;    // s_iblIrradiance
        bgfx::UniformHandle specularSampler   = BGFX_INVALID_HANDLE;    // s_iblSpecular
        bgfx::UniformHandle brdfLutSampler    = BGFX_INVALID_HANDLE;    // s_iblBrdfLut

        float specularMipCount = 1.0f;    // スペキュラのキューブのミップ数
        float intensity        = 1.0f;    // 環境光の強さ

        Gpu() = default;
        Gpu(const Gpu&)            = delete;
        Gpu& operator=(const Gpu&) = delete;

        //! テクスチャとユニフォームを破棄します。
        ~Gpu();

        //! 環境光の無いテクスチャと、ユニフォームを作ります。
        //! @return true: 作成成功
        bool CreatePlaceholder();

        //! 次に Submit する描画へ、IBL のテクスチャと定数を張ります。
        void Bind() const;
    };

}    // namespace Tsukino::Renderer

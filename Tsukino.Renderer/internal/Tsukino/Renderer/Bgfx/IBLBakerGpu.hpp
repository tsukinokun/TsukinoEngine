//----------------------------------------------------------------------------
//! @file   IBLBakerGpu.hpp
//! @brief  bgfx 版の IBLBaker の中身（Renderer の内部用）
//! @detail 環境光（IBL）のテクスチャと定数を持ち、照らすシェーダーの描画へ張ります。
//!         ベイク用のキューブマップが作れるまで（と、作れなかったとき）は、
//!         環境光の無い（黒い）1×1 のテクスチャを張ります。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/IBLBaker.hpp>

#include <bgfx/bgfx.h>

#include <array>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! IBLBaker の中身です。
    //------------------------------------------------------------------------
    struct IBLBaker::Gpu {
        static constexpr u32 kFaceCount        = 6;    //!< キューブの面の数
        static constexpr u32 kSpecularMipCount = 6;    //!< プレフィルタ済みスペキュラで焼くミップの数（IBLBaker::kSpecularMipCount と同じ）

        bgfx::TextureHandle capture    = BGFX_INVALID_HANDLE;    // スカイを焼いたキャプチャキューブ（ベイクの中間結果）
        bgfx::TextureHandle irradiance = BGFX_INVALID_HANDLE;    // 拡散の irradiance（キューブ）
        bgfx::TextureHandle specular   = BGFX_INVALID_HANDLE;    // 鏡面のプレフィルタ（キューブ）
        bgfx::TextureHandle brdfLut    = BGFX_INVALID_HANDLE;    // BRDF の積分表（2D）

        //! 書き込み先のフレームバッファ。キューブは面ごと（スペキュラは ミップ × 6 + 面 の順）に1つずつ要る
        std::array<bgfx::FrameBufferHandle, kFaceCount>                     captureFaces;
        std::array<bgfx::FrameBufferHandle, kFaceCount>                     irradianceFaces;
        std::array<bgfx::FrameBufferHandle, kSpecularMipCount * kFaceCount> specularFaces;
        bgfx::FrameBufferHandle                                             brdfLutFrameBuffer = BGFX_INVALID_HANDLE;

        bgfx::ProgramHandle irradianceProgram = BGFX_INVALID_HANDLE;    // フルスクリーン三角形 ＋ IrradianceConvolve.ps
        bgfx::ProgramHandle specularProgram   = BGFX_INVALID_HANDLE;    // フルスクリーン三角形 ＋ SpecularPrefilter.ps
        bgfx::ProgramHandle brdfLutProgram    = BGFX_INVALID_HANDLE;    // フルスクリーン三角形 ＋ BRDFIntegrate.ps

        bgfx::UniformHandle iblUniform        = BGFX_INVALID_HANDLE;    // u_ibl（x: スペキュラのミップ数, y: 強さ）
        bgfx::UniformHandle irradianceSampler = BGFX_INVALID_HANDLE;    // s_iblIrradiance
        bgfx::UniformHandle specularSampler   = BGFX_INVALID_HANDLE;    // s_iblSpecular
        bgfx::UniformHandle brdfLutSampler    = BGFX_INVALID_HANDLE;    // s_iblBrdfLut
        bgfx::UniformHandle bakeUniform       = BGFX_INVALID_HANDLE;    // u_iblBake（x: ラフネス, y: サンプル数）
        bgfx::UniformHandle captureSampler    = BGFX_INVALID_HANDLE;    // s_iblCaptureSource

        float specularMipCount = 1.0f;    // スペキュラのキューブのミップ数
        float intensity        = 1.0f;    // 環境光の強さ（キューブを焼き終えるまでは 0）

        //! 資源を持たない状態で作ります。
        Gpu();
        Gpu(const Gpu&)            = delete;
        Gpu& operator=(const Gpu&) = delete;

        //! テクスチャ・フレームバッファ・プログラム・ユニフォームを破棄します。
        ~Gpu();

        //! 環境光の無いテクスチャと、ユニフォームを作ります。
        //! @return true: 作成成功
        bool CreatePlaceholder();

        //! ベイク用のキューブマップ・BRDF の積分表と、面ごとのフレームバッファを作ります。
        //! @param  [in] captureSize    キャプチャキューブの1面の一辺
        //! @param  [in] irradianceSize irradiance キューブの1面の一辺
        //! @param  [in] specularSize   スペキュラキューブのミップ 0 の一辺
        //! @param  [in] brdfLutSize    BRDF の積分表の一辺
        //! @return true: 作成成功（仮のテクスチャは破棄して差し替える）, false: 作成失敗（仮のテクスチャのまま）
        [[nodiscard]]
        bool CreateBakeTargets(u16 captureSize, u16 irradianceSize, u16 specularSize, u16 brdfLutSize);

        //! 次に Submit する描画へ、IBL のテクスチャと定数を張ります。
        void Bind() const;
    };

}    // namespace Tsukino::Renderer

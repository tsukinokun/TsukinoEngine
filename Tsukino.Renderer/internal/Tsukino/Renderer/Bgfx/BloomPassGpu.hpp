//----------------------------------------------------------------------------
//! @file   BloomPassGpu.hpp
//! @brief  bgfx 版の BloomPass の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/BloomPass.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>
#include <Tsukino/Renderer/Bgfx/UniformBlock.hpp>

#include <bgfx/bgfx.h>

#include <array>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! BloomPass の中身です。
    //------------------------------------------------------------------------
    struct BloomPass::Gpu {
        //! 縮小段1枚分のテクスチャ
        struct MipTarget {
            bgfx::TextureHandle     texture     = BGFX_INVALID_HANDLE;    // 縮小段の絵（RGBA16F）
            bgfx::FrameBufferHandle frameBuffer = BGFX_INVALID_HANDLE;    // その絵だけを描く先にしたフレームバッファ
            u16                     width       = 0;                      // 幅
            u16                     height      = 0;                      // 高さ
        };

        bgfx::ProgramHandle prefilterProgram  = BGFX_INVALID_HANDLE;    // フルスクリーン三角形 ＋ BloomPrefilter.ps（明部抽出 + 最初の縮小）
        bgfx::ProgramHandle downsampleProgram = BGFX_INVALID_HANDLE;    // フルスクリーン三角形 ＋ BloomDownsample.ps（縮小）
        bgfx::ProgramHandle upsampleProgram   = BGFX_INVALID_HANDLE;    // フルスクリーン三角形 ＋ BloomUpsample.ps（拡大と合成）

        UniformBlock<CBufferBloom> params;                                // パラメータ（u_bloom）
        bgfx::UniformHandle        sourceSampler = BGFX_INVALID_HANDLE;    // s_bloomSource

        std::array<MipTarget, kMaxMipCount> mips{};    // 縮小段（[0] が 1/2）

        Gpu() = default;
        Gpu(const Gpu&)            = delete;
        Gpu& operator=(const Gpu&) = delete;

        //! プログラム・ユニフォーム・縮小段を破棄します。
        ~Gpu();

        //! 縮小段のテクスチャとフレームバッファを破棄します。
        void DestroyMips();
    };

}    // namespace Tsukino::Renderer

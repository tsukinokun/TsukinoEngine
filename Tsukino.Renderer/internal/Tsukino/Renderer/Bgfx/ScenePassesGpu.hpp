//----------------------------------------------------------------------------
//! @file   ScenePassesGpu.hpp
//! @brief  bgfx 版の SphereColorGradePass・AmbientParticlePass の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/AmbientParticlePass.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>
#include <Tsukino/Renderer/SphereColorGradePass.hpp>
#include <Tsukino/Renderer/Bgfx/UniformBlock.hpp>

#include <bgfx/bgfx.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! SphereColorGradePass の中身です。
    //------------------------------------------------------------------------
    struct SphereColorGradePass::Gpu {
        bgfx::ProgramHandle                   program      = BGFX_INVALID_HANDLE;    // フルスクリーン三角形 ＋ SphereColorGrade.ps
        UniformBlock<CBufferSphereColorGrade> grade;                                 // パラメータ（u_sphereColorGrade。DX11 版の b8）
        bgfx::UniformHandle                   depthSampler = BGFX_INVALID_HANDLE;    // s_gbufferDepth

        Gpu() = default;
        Gpu(const Gpu&)            = delete;
        Gpu& operator=(const Gpu&) = delete;

        //! プログラムとユニフォームを破棄します。
        ~Gpu();
    };

    //------------------------------------------------------------------------
    //! AmbientParticlePass の中身です。
    //------------------------------------------------------------------------
    struct AmbientParticlePass::Gpu {
        bgfx::ProgramHandle                  program = BGFX_INVALID_HANDLE;    // AmbientParticle.vs ＋ AmbientParticle.ps
        UniformBlock<CBufferAmbientParticle> particle;                         // パラメータ（u_ambientParticle。DX11 版の b9）

        Gpu() = default;
        Gpu(const Gpu&)            = delete;
        Gpu& operator=(const Gpu&) = delete;

        //! プログラムを破棄します。
        ~Gpu();
    };

}    // namespace Tsukino::Renderer

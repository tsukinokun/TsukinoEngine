//----------------------------------------------------------------------------
//! @file   MotionBlurPass.cpp
//! @brief  bgfx 版のモーションブラーパスの実装
//! @detail G-Buffer の速度を読み、その方向へ HDR の絵を平均して、ポストプロセス用の中間バッファへ書きます。
//!         HDR を読みながら HDR へは書けないので、書いた後はブルームとトーンマップが中間バッファを読みます。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/MotionBlurPass.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/Bgfx/MotionBlurPassGpu.hpp>
#include <Tsukino/Renderer/Bgfx/Views.hpp>

#include "FullscreenPass.hpp"

#include <Tsukino/Core/Log.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! プログラムとユニフォームを破棄します。
    //------------------------------------------------------------------------
    MotionBlurPass::Gpu::~Gpu() {
        if(!GraphicsContext::IsBgfxAlive())
            return;
        if(bgfx::isValid(program))
            bgfx::destroy(program);
        for(bgfx::UniformHandle uniform : {params, sceneSampler, velocitySampler}) {
            if(bgfx::isValid(uniform))
                bgfx::destroy(uniform);
        }
    }

    //------------------------------------------------------------------------
    //! 資源を持たない状態で作ります。
    //------------------------------------------------------------------------
    MotionBlurPass::MotionBlurPass()
        : m_gpu(std::make_unique<Gpu>()) {}

    //------------------------------------------------------------------------
    //! 資源を破棄します。
    //------------------------------------------------------------------------
    MotionBlurPass::~MotionBlurPass() = default;

    //------------------------------------------------------------------------
    //! ユニフォームとプログラムを作成します。
    //------------------------------------------------------------------------
    bool MotionBlurPass::Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants,
                                    const FullscreenPass& fullscreenPass, const Tsukino::Asset::ShaderAsset* ps) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;
        m_fullscreenPass  = &fullscreenPass;

        m_gpu->params          = bgfx::createUniform("u_motionBlur", bgfx::UniformType::Vec4);
        m_gpu->sceneSampler    = bgfx::createUniform("s_motionBlurScene", bgfx::UniformType::Sampler);
        m_gpu->velocitySampler = bgfx::createUniform("s_motionBlurVelocity", bgfx::UniformType::Sampler);

        if(!ps) {
            Tsukino::Core::Log::Error("MotionBlurPass::Initialize - shader is null.");
            return false;
        }

        m_gpu->program = fullscreenPass.CreateProgram(ps);
        if(!bgfx::isValid(m_gpu->program)) {
            Tsukino::Core::Log::Error("MotionBlurPass: The shader is not a bgfx shader; motion blur is not drawn.");
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! モーションブラーを掛けて、ポストプロセス用の中間バッファへ書きます。
    //------------------------------------------------------------------------
    bool MotionBlurPass::Execute() {
        if(!IsEnabled() || !m_fullscreenPass || !bgfx::isValid(m_gpu->program) || !bgfx::isValid(m_graphicsContext->GetPostProcessFrameBuffer()))
            return false;

        // シーンカラーは線形で、速度は点で読む
        constexpr u32 kLinearClamp = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
        constexpr u32 kPointClamp  = kLinearClamp | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT;
        bgfx::setTexture(0, m_gpu->sceneSampler, m_graphicsContext->GetHdrTexture(), kLinearClamp);
        bgfx::setTexture(1, m_gpu->velocitySampler, m_graphicsContext->GetGBufferTexture(5), kPointClamp);

        // パラメータ。サンプル数は整数のままだとビットが届くので、float にして送る
        const float params[4] = {m_data.strength, m_data.maxBlurRadius, m_data.shutterScale, static_cast<float>(m_data.sampleCount)};
        bgfx::setUniform(m_gpu->params, params);

        // 深度テストなし・ブレンドなし
        m_fullscreenPass->Submit(BgfxView::MotionBlur, m_gpu->program, BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
        return true;
    }

}    // namespace Tsukino::Renderer

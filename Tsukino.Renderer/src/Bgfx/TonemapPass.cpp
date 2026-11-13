//----------------------------------------------------------------------------
//! @file   TonemapPass.cpp
//! @brief  bgfx 版のトーンマップパスの実装
//----------------------------------------------------------------------------
#include "TonemapPass.hpp"
#include "FullscreenPass.hpp"

#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>

#include <Tsukino/Core/Log.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! プログラムとサンプラーを破棄します。
    //------------------------------------------------------------------------
    TonemapPass::~TonemapPass() {
        if(!GraphicsContext::IsBgfxAlive())
            return;
        if(bgfx::isValid(m_program))
            bgfx::destroy(m_program);
        if(bgfx::isValid(m_hdrSampler))
            bgfx::destroy(m_hdrSampler);
    }

    //------------------------------------------------------------------------
    //! プログラムとサンプラーを作成します。
    //------------------------------------------------------------------------
    bool TonemapPass::Initialize(const FullscreenPass& fullscreenPass, const Tsukino::Asset::ShaderAsset* ps) {
        m_fullscreenPass = &fullscreenPass;
        m_program        = fullscreenPass.CreateProgram(ps);
        m_hdrSampler     = bgfx::createUniform("s_hdr", bgfx::UniformType::Sampler);
        if(!bgfx::isValid(m_program)) {
            Tsukino::Core::Log::Error("TonemapPass: The tonemap shader is not a bgfx shader; the 3D scene is not shown.");
            return false;
        }
        return true;
    }

    //------------------------------------------------------------------------
    //! HDR のテクスチャをトーンマップして描きます。
    //------------------------------------------------------------------------
    void TonemapPass::Execute(bgfx::ViewId view, bgfx::TextureHandle hdr) const {
        if(!m_fullscreenPass || !bgfx::isValid(m_program) || !bgfx::isValid(hdr))
            return;

        // HDR は画面と同じ大きさなので、点で読む（DX11 版の LinearClamp と見た目は同じ）
        bgfx::setTexture(0, m_hdrSampler, hdr, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT);
        m_fullscreenPass->Submit(view, m_program, BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    }

}    // namespace Tsukino::Renderer

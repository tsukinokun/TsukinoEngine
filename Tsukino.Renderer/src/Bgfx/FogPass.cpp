//----------------------------------------------------------------------------
//! @file   FogPass.cpp
//! @brief  bgfx 版のフォグパスの実装
//! @detail 深度だけを読み、距離フォグ・高さフォグを HDR バッファへプリマルチプライの over 合成で
//!         書き込みます。半透明のモデルとワールド空間のスプライトは、深度を書かないので、
//!         自分の距離ぶんのフォグをシェーダーの中で掛けます（そのための定数を描画ごとに送ります）。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/FogPass.hpp>
#include <Tsukino/Renderer/Bgfx/FogPassGpu.hpp>
#include <Tsukino/Renderer/Bgfx/FrameConstantsGpu.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/Bgfx/TextureStages.hpp>
#include <Tsukino/Renderer/Bgfx/Views.hpp>

#include "FullscreenPass.hpp"

#include <Tsukino/Core/Log.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! プログラムとユニフォームを破棄します。
    //------------------------------------------------------------------------
    FogPass::Gpu::~Gpu() {
        if(!GraphicsContext::IsBgfxAlive())
            return;
        if(bgfx::isValid(program))
            bgfx::destroy(program);
        if(bgfx::isValid(depthSampler))
            bgfx::destroy(depthSampler);
    }

    //------------------------------------------------------------------------
    //! 次に Submit するモデル・スプライトの描画へ、フォグの定数を送ります。
    //------------------------------------------------------------------------
    void FogPass::Gpu::ApplyToDraw() const {
        static const CBufferFog kNoFog{};
        fog.Set(forwardActive ? forwardData : kNoFog);
    }

    //------------------------------------------------------------------------
    //! 資源を持たない状態で作ります。
    //------------------------------------------------------------------------
    FogPass::FogPass()
        : m_gpu(std::make_unique<Gpu>()) {}

    //------------------------------------------------------------------------
    //! 資源を破棄します。
    //------------------------------------------------------------------------
    FogPass::~FogPass() = default;

    //------------------------------------------------------------------------
    //! ユニフォームとプログラムを作成します。
    //------------------------------------------------------------------------
    bool FogPass::Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants,
                             const FullscreenPass& fullscreenPass, const Tsukino::Asset::ShaderAsset* ps) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;
        m_fullscreenPass  = &fullscreenPass;

        // 定数は、フォグパスが無くても半透明のモデルとスプライトのシェーダーが読むので、先に作る
        if(!m_gpu->fog.Create("u_fog")) {
            Tsukino::Core::Log::Error("Failed to create fog uniform.");
            return false;
        }
        m_gpu->depthSampler = bgfx::createUniform("s_gbufferDepth", bgfx::UniformType::Sampler);

        if(!ps) {
            Tsukino::Core::Log::Error("FogPass::Initialize - shader is null.");
            return false;
        }

        m_gpu->program = fullscreenPass.CreateProgram(ps);
        if(!bgfx::isValid(m_gpu->program)) {
            Tsukino::Core::Log::Error("FogPass: The fog shader is not a bgfx shader; fog is not drawn.");
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! フォグを HDR バッファへ合成します（深度だけを読むので、描く先は HDR の色だけ）。
    //------------------------------------------------------------------------
    void FogPass::Execute() {
        if(!m_enabled || !m_fullscreenPass || !bgfx::isValid(m_gpu->program))
            return;

        // 深度は画面と同じ大きさなので点で読む（DX11 版の PointClamp）
        constexpr u32 kPointClamp = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT
                                    | BGFX_SAMPLER_MIP_POINT;
        bgfx::setTexture(BgfxStage::GBufferDepth, m_gpu->depthSampler, m_graphicsContext->GetDepthTexture(), kPointClamp);

        // シーン定数（invViewProj・カメラ位置・太陽の向き）と、フォグの定数
        m_frameConstants->GetGpu().Apply();
        m_gpu->fog.Set(m_data);

        // 深度テストなし・プリマルチプライの over 合成（ONE / INV_SRC_ALPHA）
        constexpr u64 kState = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A
                               | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);
        m_fullscreenPass->Submit(BgfxView::Fog, m_gpu->program, kState);
    }

    //------------------------------------------------------------------------
    //! 半透明モデルのフォワード描画で、シェーダーの中でフォグを掛けさせます。
    //------------------------------------------------------------------------
    void FogPass::BindForwardFog() {
        if(!m_enabled)
            return;

        // フォグパスと同じ値に「シェーダーの中で掛ける」印（distanceParams.w=1）だけを立てる
        m_gpu->forwardData                  = m_data;
        m_gpu->forwardData.distanceParams.w = 1.0f;
        m_gpu->forwardActive                = true;
    }

    //------------------------------------------------------------------------
    //! BindForwardFog() で入れた定数を外します。
    //------------------------------------------------------------------------
    void FogPass::UnbindForwardFog() {
        m_gpu->forwardActive = false;
    }

}    // namespace Tsukino::Renderer

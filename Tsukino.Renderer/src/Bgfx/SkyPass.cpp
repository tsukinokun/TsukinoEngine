//----------------------------------------------------------------------------
//! @file   SkyPass.cpp
//! @brief  bgfx 版のスカイ（大気散乱）パスの実装
//! @detail フルスクリーン三角形で大気散乱のスカイを HDR バッファへ描きます。
//!         物のある画素は、この後のライティングが上書きします。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/SkyPass.hpp>
#include <Tsukino/Renderer/Bgfx/FrameConstantsGpu.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/Bgfx/ShaderPrograms.hpp>
#include <Tsukino/Renderer/Bgfx/SkyPassGpu.hpp>
#include <Tsukino/Renderer/Bgfx/Views.hpp>

#include "FullscreenPass.hpp"

#include <Tsukino/Core/Log.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! プログラムを破棄します。
    //------------------------------------------------------------------------
    SkyPass::Gpu::~Gpu() {
        if(!GraphicsContext::IsBgfxAlive())
            return;
        if(bgfx::isValid(program))
            bgfx::destroy(program);
    }

    //------------------------------------------------------------------------
    //! 資源を持たない状態で作ります。
    //------------------------------------------------------------------------
    SkyPass::SkyPass()
        : m_gpu(std::make_unique<Gpu>()) {}

    //------------------------------------------------------------------------
    //! 資源を破棄します。
    //------------------------------------------------------------------------
    SkyPass::~SkyPass() = default;

    //------------------------------------------------------------------------
    //! スカイの定数のユニフォームを作成します。
    //------------------------------------------------------------------------
    bool SkyPass::Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;

        if(!m_gpu->sky.Create("u_sky")) {
            Tsukino::Core::Log::Error("Failed to create sky uniform.");
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! スカイのシェーダーを作成して、パスを有効にします。
    //------------------------------------------------------------------------
    void SkyPass::SetPipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps) {
        if(!vs || !ps) {
            Tsukino::Core::Log::Error("SkyPass::SetPipeline - shader is null.");
            return;
        }

        // 設定し直されたときは、前のプログラムを手放してから作る
        if(bgfx::isValid(m_gpu->program)) {
            bgfx::destroy(m_gpu->program);
            m_gpu->program = BGFX_INVALID_HANDLE;
            m_hasPipeline  = false;
        }

        m_gpu->program = CreateBgfxProgram(vs, ps);
        if(!bgfx::isValid(m_gpu->program)) {
            Tsukino::Core::Log::Error("SkyPass: The sky shaders are not bgfx shaders; the sky is not drawn.");
            return;
        }

        m_hasPipeline = true;
    }

    //------------------------------------------------------------------------
    //! スカイを描きます。
    //------------------------------------------------------------------------
    void SkyPass::Execute() {
        // 描画品質で省くときは描かない。背景は、ビューを消した背景色のまま残る
        if(!m_hasPipeline || !m_drawAllowed || !m_gpu->fullscreenPass || !m_frameConstants)
            return;

        // シーン定数（invViewProj・カメラ位置・太陽の色）と、大気散乱の定数
        m_frameConstants->GetGpu().Apply();
        m_gpu->sky.Set(m_data);

        // 深度テストなし・ブレンドなし（スカイは最背面。描く先は色だけのフレームバッファ）
        m_gpu->fullscreenPass->Submit(BgfxView::Sky, m_gpu->program, BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    }

}    // namespace Tsukino::Renderer

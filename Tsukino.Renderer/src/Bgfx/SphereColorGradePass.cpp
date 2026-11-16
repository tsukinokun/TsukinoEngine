//----------------------------------------------------------------------------
//! @file   SphereColorGradePass.cpp
//! @brief  bgfx 版の球形の色調補正パスの実装
//! @detail 深度からワールド座標を復元し、指定した球の内側だけ HDR バッファの色を補正します。
//!         結果は「描かれている色 × 乗算色 ＋ 加算色」です。2つの色を同時に出すブレンド
//!         （デュアルソース）が使えない描画 API があるので、
//!         乗算と加算の2回に分けて描きます（HDR を読むためのコピーは、どちらも要りません）。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/SphereColorGradePass.hpp>
#include <Tsukino/Renderer/Bgfx/FrameConstantsGpu.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/Bgfx/ScenePassesGpu.hpp>
#include <Tsukino/Renderer/Bgfx/TextureStages.hpp>
#include <Tsukino/Renderer/Bgfx/Views.hpp>

#include "FullscreenPass.hpp"

#include <Tsukino/Core/Log.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! プログラムとユニフォームを破棄します。
    //------------------------------------------------------------------------
    SphereColorGradePass::Gpu::~Gpu() {
        if(!GraphicsContext::IsBgfxAlive())
            return;
        if(bgfx::isValid(program))
            bgfx::destroy(program);
        if(bgfx::isValid(depthSampler))
            bgfx::destroy(depthSampler);
    }

    //------------------------------------------------------------------------
    //! 資源を持たない状態で作ります。
    //------------------------------------------------------------------------
    SphereColorGradePass::SphereColorGradePass()
        : m_gpu(std::make_unique<Gpu>()) {
    }

    //------------------------------------------------------------------------
    //! 資源を破棄します。
    //------------------------------------------------------------------------
    SphereColorGradePass::~SphereColorGradePass() = default;

    //------------------------------------------------------------------------
    //! ユニフォームとプログラムを作成します。
    //------------------------------------------------------------------------
    bool SphereColorGradePass::Initialize(GraphicsContext&                   graphicsContext,
                                          RenderResources&                   resources,
                                          FrameConstants&                    frameConstants,
                                          const FullscreenPass&              fullscreenPass,
                                          const Tsukino::Asset::ShaderAsset* ps) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;
        m_fullscreenPass  = &fullscreenPass;

        if(!m_gpu->grade.Create("u_sphereColorGrade")) {
            Tsukino::Core::Log::Error("Failed to create sphere color grade uniform.");
            return false;
        }
        m_gpu->depthSampler = bgfx::createUniform("s_gbufferDepth", bgfx::UniformType::Sampler);

        if(!ps) {
            Tsukino::Core::Log::Error("SphereColorGradePass::Initialize - shader is null.");
            return false;
        }

        m_gpu->program = fullscreenPass.CreateProgram(ps);
        if(!bgfx::isValid(m_gpu->program)) {
            Tsukino::Core::Log::Error("SphereColorGradePass: The shader is not a bgfx shader; the sphere color grade is not drawn.");
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! 色調補正を HDR バッファへ掛けます（深度だけを読むので、描く先は HDR の色だけ）。
    //------------------------------------------------------------------------
    void SphereColorGradePass::Execute() {
        if(!m_enabled || !m_fullscreenPass || !bgfx::isValid(m_gpu->program))
            return;

        // 深度は画面と同じ大きさなので点で読む
        constexpr u32 kPointClamp = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT;

        //--------------------------------------------------------------------
        // 1回目：描かれている色 × 乗算色（暗くする・色を寄せる）。
        // 2回目：＋ 加算色（霧の色・境界の光）。
        // どちらもアルファはそのまま残す。ビューは積んだ順に描くので、この順で掛かる
        //--------------------------------------------------------------------
        constexpr u64 kMultiplyState =
            BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A
            | BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_SRC_COLOR, BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_ONE);
        constexpr u64 kAddState = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A
                                  | BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_ONE);

        for(const bool add : {false, true}) {
            // シェーダーが出す色の切り替え（edgeParams.y。0 = 乗算色, 1 = 加算色）
            CBufferSphereColorGrade data = m_data;
            data.edgeParams.y            = add ? 1.0f : 0.0f;

            bgfx::setTexture(BgfxStage::GBufferDepth, m_gpu->depthSampler, m_graphicsContext->GetDepthTexture(), kPointClamp);
            m_frameConstants->GetGpu().Apply();
            m_gpu->grade.Set(data);
            m_fullscreenPass->Submit(BgfxView::ColorGrade, m_gpu->program, add ? kAddState : kMultiplyState);
        }
    }

}    // namespace Tsukino::Renderer

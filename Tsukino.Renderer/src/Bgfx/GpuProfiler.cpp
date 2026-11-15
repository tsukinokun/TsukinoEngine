//----------------------------------------------------------------------------
//! @file   GpuProfiler.cpp
//! @brief  bgfx 版のパス単位の GPU タイマーの実装
//! @detail ビューの番号は Views.hpp の並びで決まっているので、番号の範囲から GpuPass を引きます。
//!         環境光のベイクのビューは DX11 版も測っていないので数えません。
//----------------------------------------------------------------------------
#include "GpuProfiler.hpp"

#include <Tsukino/Renderer/Bgfx/Views.hpp>

#include <bgfx/bgfx.h>

#include <algorithm>
#include <optional>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    namespace {
        //! ビューの番号から、時間を足し込むパスを返します。
        //! @param  [in] view ビューの番号
        //! @return パス（数えないビューなら無し）
        std::optional<GpuPass> ToGpuPass(bgfx::ViewId view) {
            using namespace BgfxView;
            if(view >= Shadow && view < GBuffer)
                return GpuPass::Shadow;
            if(view == GBuffer)
                return GpuPass::GBuffer;
            if(view == Sky)
                return GpuPass::Sky;
            if(view == Lighting)
                return GpuPass::Lighting;
            if(view == World || view == AmbientParticle)
                return GpuPass::World;
            if(view == Fog)
                return GpuPass::Fog;
            if(view == ColorGrade)
                return GpuPass::ColorGrade;
            if(view == Transparent)
                return GpuPass::Transparent;
            if(view == MotionBlur)
                return GpuPass::MotionBlur;
            if(view >= BloomPrefilter && view <= BloomComposite)
                return GpuPass::Bloom;
            if(view == Tonemap)
                return GpuPass::Tonemap;
            if(view == Effects || view == EffectsPresent)
                return GpuPass::Effects;
            if(view >= Overlay && view <= OverlayLast)
                return GpuPass::Overlay;
            return std::nullopt;
        }
    }    // namespace

    //------------------------------------------------------------------------
    //! bgfx にビューごとの統計を集めさせます。
    //------------------------------------------------------------------------
    void GpuProfiler::Initialize() {
        bgfx::setDebug(BGFX_DEBUG_PROFILER);
        for(std::size_t i = 0; i < kGpuPassCount; ++i)
            m_latest.passes[i].name = GetGpuPassName(static_cast<GpuPass>(i));
    }

    //------------------------------------------------------------------------
    //! bgfx の統計から、届いているフレームの GPU 時間を読み出します。
    //------------------------------------------------------------------------
    const GpuFrameTimings& GpuProfiler::Collect() {
        const bgfx::Stats* stats = bgfx::getStats();
        if(!stats || stats->gpuTimerFreq <= 0 || stats->numViews == 0)
            return m_latest;

        const double toMs = 1000.0 / static_cast<double>(stats->gpuTimerFreq);

        GpuFrameTimings timings;
        for(std::size_t i = 0; i < kGpuPassCount; ++i)
            timings.passes[i].name = GetGpuPassName(static_cast<GpuPass>(i));

        //--------------------------------------------------------------------
        // フレーム全体は、最初のビューの開始から最後のビューの終了まで。
        // bgfx の Stats::gpuTimeBegin / End は垂直同期の待ちまで含むので使わない（DX11 版は描画の前後で測っている）
        //--------------------------------------------------------------------
        i64 frameBegin = 0;
        i64 frameEnd   = 0;
        for(u16 i = 0; i < stats->numViews; ++i) {
            const bgfx::ViewStats& view = stats->viewStats[i];
            if(view.gpuTimeEnd <= view.gpuTimeBegin)
                continue;
            if(frameEnd == 0 || view.gpuTimeBegin < frameBegin)
                frameBegin = view.gpuTimeBegin;
            frameEnd = std::max(frameEnd, view.gpuTimeEnd);

            if(const std::optional<GpuPass> pass = ToGpuPass(view.view))
                timings.passes[static_cast<std::size_t>(*pass)].ms += static_cast<float>(static_cast<double>(view.gpuTimeEnd - view.gpuTimeBegin) * toMs);
        }
        if(frameEnd <= frameBegin)
            return m_latest;

        timings.frameMs = static_cast<float>(static_cast<double>(frameEnd - frameBegin) * toMs);
        timings.valid   = true;
        m_latest        = timings;
        return m_latest;
    }

}    // namespace Tsukino::Renderer

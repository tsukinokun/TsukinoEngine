//----------------------------------------------------------------------------
//! @file   AmbientParticlePass.cpp
//! @brief  bgfx 版の環境パーティクル（火の粉・灰）パスの実装
//! @detail 頂点バッファもインデックスバッファも持たず、1回の描画で粒子数×6頂点を出します。
//!         粒子の属性はすべて頂点シェーダーが頂点番号のハッシュから作るので、送るのはパラメータだけです。
//!         深度テストありで HDR バッファへ加算するので、世界の物体に隠れ、フォグとトーンマップの両方が乗ります。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/AmbientParticlePass.hpp>
#include <Tsukino/Renderer/Bgfx/FrameConstantsGpu.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/Bgfx/ScenePassesGpu.hpp>
#include <Tsukino/Renderer/Bgfx/ShaderPrograms.hpp>
#include <Tsukino/Renderer/Bgfx/Views.hpp>

#include <Tsukino/Core/Log.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! プログラムを破棄します。
    //------------------------------------------------------------------------
    AmbientParticlePass::Gpu::~Gpu() {
        if(!GraphicsContext::IsBgfxAlive())
            return;
        if(bgfx::isValid(program))
            bgfx::destroy(program);
    }

    //------------------------------------------------------------------------
    //! 資源を持たない状態で作ります。
    //------------------------------------------------------------------------
    AmbientParticlePass::AmbientParticlePass()
        : m_gpu(std::make_unique<Gpu>()) {
    }

    //------------------------------------------------------------------------
    //! 資源を破棄します。
    //------------------------------------------------------------------------
    AmbientParticlePass::~AmbientParticlePass() = default;

    //------------------------------------------------------------------------
    //! ユニフォームとプログラムを作成します。
    //------------------------------------------------------------------------
    bool AmbientParticlePass::Initialize(GraphicsContext&                   graphicsContext,
                                         RenderResources&                   resources,
                                         FrameConstants&                    frameConstants,
                                         const Tsukino::Asset::ShaderAsset* vs,
                                         const Tsukino::Asset::ShaderAsset* ps) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;

        if(!m_gpu->particle.Create("u_ambientParticle")) {
            Tsukino::Core::Log::Error("Failed to create ambient particle uniform.");
            return false;
        }

        if(!vs || !ps) {
            Tsukino::Core::Log::Error("AmbientParticlePass::Initialize - shader is null.");
            return false;
        }

        m_gpu->program = CreateBgfxProgram(vs, ps);
        if(!bgfx::isValid(m_gpu->program)) {
            Tsukino::Core::Log::Error("AmbientParticlePass: The shaders are not bgfx shaders; ambient particles are not drawn.");
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! 環境パーティクルを HDR バッファへ加算合成で描きます。
    //------------------------------------------------------------------------
    void AmbientParticlePass::Execute() {
        if(!m_enabled || !bgfx::isValid(m_gpu->program) || m_count == 0)
            return;

        // シーン定数（ビュー行列・カメラ位置を頂点シェーダーで使う）と、パラメータ
        m_frameConstants->GetGpu().Apply();
        m_gpu->particle.Set(m_data);

        //--------------------------------------------------------------------
        // 深度テストあり（リバース Z なので GREATER_EQUAL）・深度書き込みなし・加算合成（SRC_ALPHA / ONE）。
        // 深度を書かないことで粒子どうしの前後関係を気にせずに済み、加算合成なので描画順のソートも要らない。
        // カリングはしない（板の巻き方向を問わない）
        //--------------------------------------------------------------------
        constexpr u64 kState =
            BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_GEQUAL | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE);

        // 頂点バッファなしで「1粒 = 三角形2枚」を一括描画（頂点シェーダーが頂点番号から作る）
        bgfx::setVertexCount(m_count * 6);
        bgfx::setState(kState);
        bgfx::submit(BgfxView::AmbientParticle, m_gpu->program);
    }

}    // namespace Tsukino::Renderer

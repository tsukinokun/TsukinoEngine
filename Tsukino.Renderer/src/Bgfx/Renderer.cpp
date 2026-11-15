//----------------------------------------------------------------------------
//! @file   Renderer.cpp
//! @brief  bgfx 版の Renderer の実装（作りかけ）
//! @detail DX11 版と同じ順に、不透明なモデルを G-Buffer へ描き、照らして HDR バッファへ書き、
//!         フォワードの 3D（World・半透明）を重ね、トーンマップしてバックバッファへ写し、
//!         最後に UI（Overlay）を描きます。影はシャドウマップで落とし、背景にはスカイを描き、環境光はスカイから焼きます。後処理はまだ無く、
//!         DX11 版の各パスを段階的にここへ移していきます。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/Bgfx/FrameConstantsGpu.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/BloomPass.hpp>
#include <Tsukino/Renderer/Bgfx/IBLBakerGpu.hpp>
#include <Tsukino/Renderer/Bgfx/SkyPassGpu.hpp>
#include <Tsukino/Renderer/Bgfx/Views.hpp>

#include "DrawCommandExecutor.hpp"
#include "FullscreenPass.hpp"
#include "ShadowPass.hpp"
#include "GpuProfiler.hpp"
#include "TonemapPass.hpp"

#include <Tsukino/Core/Log.hpp>

#include <bgfx/bgfx.h>

#include <algorithm>
#include <cmath>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    namespace {
        //! 0〜1 の色の成分を 8bit に直します。
        //! @param  [in] value 色の成分
        //! @return 0〜255
        u32 ToByte(float value) {
            return static_cast<u32>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
        }

        //! ビューを、積んだ順のまま描く（bgfx は既定でステートの順に並べ替える）ように設定します。
        //! @param  [in] view        ビュー
        //! @param  [in] frameBuffer 描く先（無効なハンドルならバックバッファ）
        void SetupView(bgfx::ViewId view, bgfx::FrameBufferHandle frameBuffer) {
            bgfx::setViewMode(view, bgfx::ViewMode::Sequential);
            bgfx::setViewFrameBuffer(view, frameBuffer);
            bgfx::setViewRect(view, 0, 0, bgfx::BackbufferRatio::Equal);
        }
    }    // namespace

    //------------------------------------------------------------------------
    //! コンストラクタ
    //------------------------------------------------------------------------
    Renderer::Renderer()
        : m_graphicsContext(std::make_unique<GraphicsContext>()) {}

    //------------------------------------------------------------------------
    //! デストラクタ
    //------------------------------------------------------------------------
    Renderer::~Renderer() = default;

    //------------------------------------------------------------------------
    //! レンダラーを初期化します。
    //------------------------------------------------------------------------
    bool Renderer::Initialize(const NativeWindow& window, uint32_t width, uint32_t height, const RendererShaderSet& shaders,
                              const GraphicsDeviceOptions& deviceOptions) {
        if(!m_graphicsContext->Initialize(window.handle, width, height, deviceOptions))
            return false;

        if(!m_resources.Initialize(*m_graphicsContext))
            return false;

        if(!m_frameConstants.Initialize(*m_graphicsContext))
            return false;

        // 環境光の定数と、焼くまでの仮の（環境光の無い）テクスチャ。照らすシェーダーが読むので、先に作る
        if(!m_iblBaker.CreateConstantBuffers(*m_graphicsContext))
            return false;

        m_commandExecutor = std::make_unique<DrawCommandExecutor>();
        if(!m_commandExecutor->Initialize(*m_graphicsContext, m_resources, m_frameConstants, m_drawQueue, m_iblBaker, shaders.debugVS,
                                          shaders.debugPS))
            return false;

        //--------------------------------------------------------------------
        // シャドウマップ。作れなくても起動は止めない（影が出ないだけ）
        //--------------------------------------------------------------------
        m_shadowPass = std::make_unique<ShadowPass>();
        if(!m_shadowPass->Initialize(m_resources, m_frameConstants, *m_commandExecutor, shaders.shadowStaticVS, shaders.shadowSkeletalVS,
                                     shaders.shadowPS))
            Tsukino::Core::Log::Error("Renderer: Failed to create the shadow pass; shadows are not drawn.");
        m_commandExecutor->SetShadowPass(m_shadowPass.get());
        m_commandExecutor->SetFogPass(&m_fogPass);

        // 環境パーティクル。作れなくても起動は止めない（粒子が出ないだけ）
        if(!m_ambientParticlePass.Initialize(*m_graphicsContext, m_resources, m_frameConstants, shaders.ambientParticleVS, shaders.ambientParticlePS))
            Tsukino::Core::Log::Error("Renderer: Ambient particles are disabled because their resources could not be created.");

        //--------------------------------------------------------------------
        // フルスクリーン三角形・トーンマップ・ライティング。
        // パス単位の GPU タイマー（bgfx がビューごとに測った時間を読む）
        m_gpuProfiler = std::make_unique<GpuProfiler>();
        m_gpuProfiler->Initialize();

        // シェーダーがまだ bgfx の形式でなくても、起動は止めない（3D が出ないだけ）
        //--------------------------------------------------------------------
        m_fullscreenPass = std::make_unique<FullscreenPass>();

        // スカイ。シェーダーは SkyAtmosphereSystem が SetPipeline() で渡すまで無く、それまでは描かない
        if(!m_skyPass.Initialize(*m_graphicsContext, m_resources, m_frameConstants))
            Tsukino::Core::Log::Error("Renderer: Failed to create the sky pass; the sky is not drawn.");
        m_skyPass.GetGpu().fullscreenPass = m_fullscreenPass.get();

        m_tonemapPass    = std::make_unique<TonemapPass>();
        if(m_fullscreenPass->Initialize(shaders.tonemapVS)) {
            // 環境光のベイク。作れなくても起動は止めない（環境光が無いだけ）
            if(!m_iblBaker.Initialize(*m_graphicsContext, m_resources, m_frameConstants, *m_fullscreenPass, m_skyPass, shaders.iblIrradiancePS,
                                      shaders.iblSpecularPrefilterPS, shaders.iblBRDFLUTPS))
                Tsukino::Core::Log::Warn("Renderer: IBL initialization failed. Ambient lighting will be disabled.");

            // フォグ。作れなくても起動は止めない（霧が出ないだけ）
            if(!m_fogPass.Initialize(*m_graphicsContext, m_resources, m_frameConstants, *m_fullscreenPass, shaders.fogPS))
                Tsukino::Core::Log::Error("Renderer: Fog is disabled because its resources could not be created.");

            // 球形の色調補正。作れなくても起動は止めない（補正が掛からないだけ）
            if(!m_sphereColorGradePass.Initialize(*m_graphicsContext, m_resources, m_frameConstants, *m_fullscreenPass, shaders.sphereColorGradePS))
                Tsukino::Core::Log::Error("Renderer: Sphere color grade is disabled because its resources could not be created.");

            // モーションブラー。作れなくても起動は止めない（ぼけないだけ）
            if(!m_motionBlurPass.Initialize(*m_graphicsContext, m_resources, m_frameConstants, *m_fullscreenPass, shaders.motionBlurPS))
                Tsukino::Core::Log::Error("Renderer: Motion blur is disabled because its resources could not be created.");

            // ブルーム。作れなくても起動は止めない（光が滲まないだけ）
            if(!m_bloomPass.Initialize(*m_graphicsContext, m_resources, *m_fullscreenPass, shaders.bloomPrefilterPS, shaders.bloomDownsamplePS,
                                       shaders.bloomUpsamplePS))
                Tsukino::Core::Log::Error("Renderer: Bloom is disabled because its resources could not be created.");

            (void)m_tonemapPass->Initialize(*m_fullscreenPass, shaders.tonemapPS);
            (void)m_lightingPass.Initialize(*m_graphicsContext, m_resources, m_frameConstants, *m_fullscreenPass, *m_shadowPass, m_iblBaker,
                                            shaders.lightingPS);
        }

        //--------------------------------------------------------------------
        // ビュー（Views.hpp の順に描かれる）。深度はリバース Z（手前ほど大きい）なので 0 で消す
        //--------------------------------------------------------------------
        SetupView(BgfxView::GBuffer, m_graphicsContext->GetGBufferFrameBuffer());
        SetupView(BgfxView::Sky, m_graphicsContext->GetHdrColorFrameBuffer());
        SetupView(BgfxView::Lighting, m_graphicsContext->GetHdrColorFrameBuffer());
        SetupView(BgfxView::World, m_graphicsContext->GetHdrFrameBuffer());
        SetupView(BgfxView::AmbientParticle, m_graphicsContext->GetHdrFrameBuffer());
        SetupView(BgfxView::Fog, m_graphicsContext->GetHdrColorFrameBuffer());
        SetupView(BgfxView::ColorGrade, m_graphicsContext->GetHdrColorFrameBuffer());
        SetupView(BgfxView::Transparent, m_graphicsContext->GetHdrFrameBuffer());
        SetupView(BgfxView::MotionBlur, m_graphicsContext->GetPostProcessFrameBuffer());
        SetupView(BgfxView::Tonemap, BGFX_INVALID_HANDLE);
        bgfx::setViewClear(BgfxView::GBuffer, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0, 0.0f, 0);
        return true;
    }

    //------------------------------------------------------------------------
    //! 1フレームを描きます。
    //------------------------------------------------------------------------
    void Renderer::Render(IPostWorldPass* /*postWorldPass*/) {
        if(m_qualityDirty)
            ApplyQuality();

        m_frameStats              = FrameStats{};
        m_frameStats.commandCount = static_cast<u32>(m_drawQueue.GetCommands().size());

        const auto& commands = m_drawQueue.GetCommands();

        // モーションブラーが有効なフレームだけ、前フレームの値を持つコマンドに速度を出させる
        m_commandExecutor->SetMotionBlurEnabled(m_motionBlurPass.IsEnabled());

        //--------------------------------------------------------------------
        // 背景。HDR バッファを背景色で消すのは、スカイのビュー（スカイを描かないときは背景色が残る）。
        // G-Buffer に何も描かれていない画素は、ライティングが背景のまま残す
        //--------------------------------------------------------------------
        const u32 rgba = (ToByte(m_clearColor[0]) << 24) | (ToByte(m_clearColor[1]) << 16) | (ToByte(m_clearColor[2]) << 8)
                         | ToByte(m_clearColor[3]);
        bgfx::setViewClear(BgfxView::Sky, BGFX_CLEAR_COLOR, rgba, 0.0f, 0);
        bgfx::touch(BgfxView::GBuffer);
        bgfx::touch(BgfxView::Sky);
        bgfx::touch(BgfxView::Lighting);
        bgfx::touch(BgfxView::Tonemap);

        //--------------------------------------------------------------------
        // 影のカスケード（メインカメラの視錐台を奥行きで切った1切れごとの投影）を作り、
        m_frameStats.gpu          = m_gpuProfiler->Collect();
        // 影を落とす物（GBuffer パスの対象＝不透明な 3D モデル）をシャドウマップへ描く。
        // 以降のライティングとフォワードが同じカスケードを使う
        //--------------------------------------------------------------------
        m_shadowPass->UpdateCascades(m_frameConstants.GetWorldSceneData());
        m_shadowPass->Execute(commands, m_frameStats);

        //--------------------------------------------------------------------
        // GBuffer パス（不透明な 3D モデル）→ スカイ → ライティング
        //--------------------------------------------------------------------
        m_frameConstants.UploadWorld();
        for(const DrawCommand& cmd : commands) {
            if(cmd.pass == RenderPass::GBuffer)
                m_commandExecutor->Execute(cmd, BgfxView::GBuffer, m_frameStats);
        }

        // スカイ（最背面）。物のある画素は、この後のライティングが上書きする
        m_skyPass.Execute();

        //--------------------------------------------------------------------
        // 環境光のベイク（スカイのキャプチャ → irradiance の畳み込み → スペキュラのプレフィルタ）。
        // スカイのパイプラインが確立した最初のフレームで一度だけ走る（RequestRecapture() で焼き直せる）。
        // ベイクのビューはライティングより番号が小さいので、焼いた結果は同じフレームのライティングから読める
        //--------------------------------------------------------------------
        m_iblBaker.BakeIfNeeded();

        m_lightingPass.Execute();

        //--------------------------------------------------------------------
        // フォワードの 3D。World（深度を書く物）を描く
        //--------------------------------------------------------------------
        for(const DrawCommand& cmd : commands) {
            if(cmd.pass == RenderPass::World)
                m_commandExecutor->Execute(cmd, BgfxView::World, m_frameStats);
        }

        //--------------------------------------------------------------------
        // 環境パーティクル（HDR へ加算合成）。深度テストありなので世界の物体に隠れる。
        // フォグの前に置くことで、粒子にもフォグが乗る
        //--------------------------------------------------------------------
        m_ambientParticlePass.Execute();

        //--------------------------------------------------------------------
        // フォグ（HDR へ直接 over 合成）。
        // 不透明・空・環境パーティクルを描き終え、まだ半透明を描く前のここで掛ける。フォグは深度から
        // 画素の距離を求めるので、半透明の深度の事前パスより後に掛けると、半透明のモデルが重なった画素の
        // 奥の景色にモデルの距離でフォグが掛かってしまう
        //--------------------------------------------------------------------
        m_fogPass.Execute();

        //--------------------------------------------------------------------
        // 球形の色調補正（フォグの上から掛ける）。フォグと同じく深度から画素の位置を求めるので、
        // 半透明の深度の事前パスより前に置く。霧に包まれた遠景まで同じ色合いに揃えるため、フォグの後に掛ける
        //--------------------------------------------------------------------
        m_sphereColorGradePass.Execute();

        //--------------------------------------------------------------------
        // 半透明（深度の事前パス → 色）。半透明のモデルとワールド空間のスプライトは、
        // 色のパスの間だけ、自分の距離ぶんのフォグをシェーダーの中で掛ける
        //--------------------------------------------------------------------
        for(const RenderPass pass : {RenderPass::TransparentDepth, RenderPass::Transparent}) {
            if(pass == RenderPass::Transparent)
                m_fogPass.BindForwardFog();
            for(const DrawCommand& cmd : commands) {
                if(cmd.pass == pass)
                    m_commandExecutor->Execute(cmd, BgfxView::Transparent, m_frameStats);
            }
        }
        m_fogPass.UnbindForwardFog();

        //--------------------------------------------------------------------
        // モーションブラー（HDR → ポストプロセス用の中間バッファ）。
        // 3D の描画がすべて終わり、UI が乗る前のここで掛ける。無効なら何もせず false を返すので、
        // その場合は HDR をそのままトーンマップへ渡す
        //--------------------------------------------------------------------
        const bool motionBlurred = m_motionBlurPass.Execute();

        //--------------------------------------------------------------------
        // ブルーム（トーンマップの入力へ加算）→ トーンマップ。
        // ブルームはモーションブラーの後に掛ける。先に掛けると、滲んだ光の裾まで
        // 速度の向きへ引き伸ばされて輪郭のない筋になる
        //--------------------------------------------------------------------
        m_bloomPass.Execute(motionBlurred);
        m_tonemapPass->Execute(BgfxView::Tonemap, motionBlurred ? m_graphicsContext->GetPostProcessTexture() : m_graphicsContext->GetHdrTexture());

        //--------------------------------------------------------------------
        // Overlay パス。DX11 版と同じく sortOrder の昇順（同じ値は積んだ順）に並べて描く
        //--------------------------------------------------------------------
        m_overlayOrder.clear();
        for(u32 index = 0; index < static_cast<u32>(commands.size()); ++index) {
            if(commands[index].pass == RenderPass::Overlay)
                m_overlayOrder.push_back(index);
        }
        std::sort(m_overlayOrder.begin(), m_overlayOrder.end(), [&commands](u32 lhs, u32 rhs) {
            if(commands[lhs].sortOrder != commands[rhs].sortOrder)
                return commands[lhs].sortOrder < commands[rhs].sortOrder;
            return lhs < rhs;
        });

        //--------------------------------------------------------------------
        // UI の層の 3D モデル（cameraOverride を持つコマンド）は、そのカメラに差し替えて描く。
        // sortOrder の違うモデルは重ね順で前後を決めるので、sortOrder が変わるたびに深度を消し直す。
        // bgfx はビューの途中で深度を消せないので、そのたびに次の番号のビュー（深度を消すビュー）へ移る
        //--------------------------------------------------------------------
        bgfx::ViewId overlayView = BgfxView::Overlay;
        SetupView(overlayView, BGFX_INVALID_HANDLE);
        bgfx::setViewClear(overlayView, BGFX_CLEAR_DEPTH, 0, 0.0f, 0);

        m_frameConstants.UploadOverlay();
        bool overlayDepthUsed      = false;    // 今のビューの深度をモデルが使ったか
        int  overlayDepthSortOrder = 0;        // 今の深度を使っているモデルの sortOrder
        bool overlayCameraOnLoan   = false;    // シーン定数のカメラを差し替えているか
        for(u32 index : m_overlayOrder) {
            const DrawCommand& cmd = commands[index];
            if(cmd.cameraOverride) {
                if(!overlayDepthUsed) {
                    overlayDepthUsed      = true;
                    overlayDepthSortOrder = cmd.sortOrder;
                } else if(cmd.sortOrder != overlayDepthSortOrder && overlayView < BgfxView::OverlayLast) {
                    ++overlayView;
                    SetupView(overlayView, BGFX_INVALID_HANDLE);
                    bgfx::setViewClear(overlayView, BGFX_CLEAR_DEPTH, 0, 0.0f, 0);
                    overlayDepthSortOrder = cmd.sortOrder;
                }
                m_frameConstants.UploadWorldWithCamera(*cmd.cameraOverride);
                overlayCameraOnLoan = true;
            } else if(overlayCameraOnLoan) {
                m_frameConstants.UploadOverlay();
                overlayCameraOnLoan = false;
            }
            m_commandExecutor->Execute(cmd, overlayView, m_frameStats);
        }

        bgfx::frame();

        // 描画コマンドと、Submit されずに残ったデバッグ線は、DX11 版と同じくフレームの終わりで捨てる
        m_drawQueue.Clear();
        m_debugDraw.Clear();

        // 次フレームの速度計算用に、今フレームの ViewProjection を退避する（DX11 版と同じ）
        m_frameConstants.EndFrame();

        // 各パスの「このフレームだけ有効」の印を戻す（DX11 版の Render() の終わりと揃える）
        m_fogPass.EndFrame();
        m_sphereColorGradePass.EndFrame();
        m_motionBlurPass.EndFrame();
        m_bloomPass.EndFrame();
        m_ambientParticlePass.EndFrame();
    }

    //------------------------------------------------------------------------
    //! 描画領域の大きさを変えます。
    //------------------------------------------------------------------------
    void Renderer::Resize(uint32_t width, uint32_t height) {
        m_drawQueue.Clear();
        m_debugDraw.Clear();
        m_graphicsContext->Resize(width, height);
    }

    //------------------------------------------------------------------------
    //! 背景色を設定します。
    //------------------------------------------------------------------------
    void Renderer::SetClearColor(float r, float g, float b, float a) {
        m_clearColor = {r, g, b, a};
    }

    //------------------------------------------------------------------------
    //! 影を落とす物の間引きを切り替えます。
    //------------------------------------------------------------------------
    void Renderer::SetShadowCullingEnabled(bool enabled) {
        if(m_shadowPass)
            m_shadowPass->SetCullingEnabled(enabled);
    }

    //------------------------------------------------------------------------
    //! 影の設定を変えます。
    //------------------------------------------------------------------------
    void Renderer::SetShadowSettings(const ShadowSettings& settings) {
        if(m_shadowPass)
            m_shadowPass->SetSettings(settings);
    }

    //------------------------------------------------------------------------
    //! 影の設定を取得します。
    //------------------------------------------------------------------------
    const ShadowSettings& Renderer::GetShadowSettings() const {
        static const ShadowSettings kDefaultSettings{};
        return m_shadowPass ? m_shadowPass->GetSettings() : kDefaultSettings;
    }

    //------------------------------------------------------------------------
    //! 描画品質を設定します。
    //------------------------------------------------------------------------
    void Renderer::SetQuality(const RenderQualitySettings& settings) {
        m_quality             = settings;
        m_quality.renderScale = std::clamp(settings.renderScale, RenderQualitySettings::kMinRenderScale, 1.0f);
        m_qualityDirty        = true;
    }

    //------------------------------------------------------------------------
    //! 保留している描画品質を適用します（内部解像度は、bgfx 版ではまだ効かない）。
    //------------------------------------------------------------------------
    void Renderer::ApplyQuality() {
        m_qualityDirty = false;

        if(m_shadowPass)
            m_shadowPass->SetQuality(m_quality.shadowMapSizeLimit, m_quality.shadowCascadeCount);

        m_frameConstants.SetShadowSoftFilter(m_quality.shadowSoftFilter);
        m_skyPass.SetDrawAllowed(m_quality.drawSky);
        m_bloomPass.SetMipLimit(m_quality.bloomMipCount);
        m_motionBlurPass.SetAllowed(m_quality.motionBlur);
    }

    //------------------------------------------------------------
    //! 垂直同期の有無を設定します。
    //------------------------------------------------------------
    void Renderer::SetVSyncEnabled(bool enabled) {
        m_graphicsContext->SetVSyncEnabled(enabled);
    }

    //------------------------------------------------------------
    //! 垂直同期が有効かを取得します。
    //------------------------------------------------------------
    bool Renderer::IsVSyncEnabled() const {
        return m_graphicsContext->IsVSyncEnabled();
    }

    //------------------------------------------------------------
    //! 使っている GPU の情報を取得します。
    //------------------------------------------------------------
    const GraphicsAdapterInfo& Renderer::GetAdapterInfo() const noexcept {
        return m_graphicsContext->GetAdapterInfo();
    }

    //------------------------------------------------------------
    //! 描画 API の生のデバイスを取得します。
    //! @note  bgfx 版はデバイスコンテキストを外へ出さないので、context は nullptr になる
    //!        （Effekseer の DX11 版はこのとき動かない）
    //------------------------------------------------------------
    NativeGraphicsContext Renderer::GetNativeGraphicsContext() const noexcept {
        const bgfx::InternalData* data = bgfx::getInternalData();
        return NativeGraphicsContext{data ? data->context : nullptr, nullptr};
    }

}    // namespace Tsukino::Renderer

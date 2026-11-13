//----------------------------------------------------------------------------
//! @file   Renderer.cpp
//! @brief  bgfx 版の Renderer の実装（作りかけ）
//! @detail DX11 版と同じ順に、不透明なモデルを G-Buffer へ描き、照らして HDR バッファへ書き、
//!         フォワードの 3D（World・半透明）を重ね、トーンマップしてバックバッファへ写し、
//!         最後に UI（Overlay）を描きます。影・スカイ・環境光のベイク・後処理はまだ無く、
//!         DX11 版の各パスを段階的にここへ移していきます。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/Bgfx/FrameConstantsGpu.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>
#include <Tsukino/Renderer/Bgfx/IBLBakerGpu.hpp>
#include <Tsukino/Renderer/Bgfx/Views.hpp>

#include "DrawCommandExecutor.hpp"
#include "FullscreenPass.hpp"
#include "TonemapPass.hpp"

#include <bgfx/bgfx.h>

#include <algorithm>
#include <cmath>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    // Renderer.hpp が前方宣言だけで持っている内部の部品。
    // bgfx 版ではまだ中身が無いので、空の型として定義しておく
    //------------------------------------------------------------------------

    //! パス単位の GPU タイマー（bgfx 版はまだ無い）
    class GpuProfiler {};

    //------------------------------------------------------------------------
    //! シャドウマップパスです（bgfx 版は設定を覚えておくだけ）。
    //------------------------------------------------------------------------
    class ShadowPass {
    public:
        ShadowSettings settings;    // 影の設定
    };

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

        // 環境光。スカイからのベイク（段階 1-4 c）を移すまでは、環境光の無い仮のテクスチャ
        if(!m_iblBaker.GetGpu().CreatePlaceholder())
            return false;

        m_commandExecutor = std::make_unique<DrawCommandExecutor>();
        if(!m_commandExecutor->Initialize(*m_graphicsContext, m_resources, m_frameConstants, m_drawQueue, m_iblBaker, shaders.debugVS,
                                          shaders.debugPS))
            return false;

        //--------------------------------------------------------------------
        // フルスクリーン三角形・トーンマップ・ライティング。
        // シェーダーがまだ bgfx の形式でなくても、起動は止めない（3D が出ないだけ）
        //--------------------------------------------------------------------
        m_shadowPass     = std::make_unique<ShadowPass>();
        m_fullscreenPass = std::make_unique<FullscreenPass>();
        m_tonemapPass    = std::make_unique<TonemapPass>();
        if(m_fullscreenPass->Initialize(shaders.tonemapVS)) {
            (void)m_tonemapPass->Initialize(*m_fullscreenPass, shaders.tonemapPS);
            (void)m_lightingPass.Initialize(*m_graphicsContext, m_resources, m_frameConstants, *m_fullscreenPass, *m_shadowPass, m_iblBaker,
                                            shaders.lightingPS);
        }

        //--------------------------------------------------------------------
        // ビュー（Views.hpp の順に描かれる）。深度はリバース Z（手前ほど大きい）なので 0 で消す
        //--------------------------------------------------------------------
        SetupView(BgfxView::GBuffer, m_graphicsContext->GetGBufferFrameBuffer());
        SetupView(BgfxView::Lighting, m_graphicsContext->GetHdrColorFrameBuffer());
        SetupView(BgfxView::World, m_graphicsContext->GetHdrFrameBuffer());
        SetupView(BgfxView::Transparent, m_graphicsContext->GetHdrFrameBuffer());
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

        //--------------------------------------------------------------------
        // 背景。G-Buffer に何も描かれていない画素は、ライティングが背景色のまま残す
        // （DX11 版は HDR バッファを背景色で消している。スカイは段階 1-4 c）
        //--------------------------------------------------------------------
        const u32 rgba = (ToByte(m_clearColor[0]) << 24) | (ToByte(m_clearColor[1]) << 16) | (ToByte(m_clearColor[2]) << 8)
                         | ToByte(m_clearColor[3]);
        bgfx::setViewClear(BgfxView::Lighting, BGFX_CLEAR_COLOR, rgba, 0.0f, 0);
        bgfx::touch(BgfxView::GBuffer);
        bgfx::touch(BgfxView::Lighting);
        bgfx::touch(BgfxView::Tonemap);

        //--------------------------------------------------------------------
        // GBuffer パス（不透明な 3D モデル）→ ライティング
        //--------------------------------------------------------------------
        m_frameConstants.UploadWorld();
        for(const DrawCommand& cmd : commands) {
            if(cmd.pass == RenderPass::GBuffer)
                m_commandExecutor->Execute(cmd, BgfxView::GBuffer, m_frameStats);
        }
        m_lightingPass.Execute();

        //--------------------------------------------------------------------
        // フォワードの 3D。World（深度を書く物）→ 半透明（深度の事前パス → 色）。
        // フォグ・環境パーティクル・球形の色調補正（段階 1-4 d）はまだ無い
        //--------------------------------------------------------------------
        for(const DrawCommand& cmd : commands) {
            if(cmd.pass == RenderPass::World)
                m_commandExecutor->Execute(cmd, BgfxView::World, m_frameStats);
        }
        for(const RenderPass pass : {RenderPass::TransparentDepth, RenderPass::Transparent}) {
            for(const DrawCommand& cmd : commands) {
                if(cmd.pass == pass)
                    m_commandExecutor->Execute(cmd, BgfxView::Transparent, m_frameStats);
            }
        }

        //--------------------------------------------------------------------
        // トーンマップ（モーションブラーとブルームは段階 1-4 d）
        //--------------------------------------------------------------------
        m_tonemapPass->Execute(BgfxView::Tonemap, m_graphicsContext->GetHdrTexture());

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
    //! 影を落とす物の間引きを切り替えます（bgfx 版はまだ影を描かない）。
    //------------------------------------------------------------------------
    void Renderer::SetShadowCullingEnabled(bool /*enabled*/) {}

    //------------------------------------------------------------------------
    //! 影の設定を変えます。
    //------------------------------------------------------------------------
    void Renderer::SetShadowSettings(const ShadowSettings& settings) {
        if(m_shadowPass)
            m_shadowPass->settings = settings;
    }

    //------------------------------------------------------------------------
    //! 影の設定を取得します。
    //------------------------------------------------------------------------
    const ShadowSettings& Renderer::GetShadowSettings() const {
        static const ShadowSettings kDefaultSettings{};
        return m_shadowPass ? m_shadowPass->settings : kDefaultSettings;
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
    //! 保留している描画品質を適用します（bgfx 版はまだ効く先が無い）。
    //------------------------------------------------------------------------
    void Renderer::ApplyQuality() {
        m_qualityDirty = false;
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

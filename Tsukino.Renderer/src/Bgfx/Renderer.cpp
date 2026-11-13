//----------------------------------------------------------------------------
//! @file   Renderer.cpp
//! @brief  bgfx 版の Renderer の実装（作りかけ）
//! @detail bgfx を初期化して、毎フレーム背景色で塗り、描画コマンドのうち
//!         World・Transparent・Overlay のパスにあるものをバックバッファへ直接描きます。
//!         GBuffer・ライティング・影・後処理はまだ無く、DX11 版の各パスを段階的にここへ移していきます。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/Bgfx/FrameConstantsGpu.hpp>
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>

#include "DrawCommandExecutor.hpp"

#include <bgfx/bgfx.h>

#include <algorithm>
#include <cmath>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    // Renderer.hpp が前方宣言だけで持っている内部の部品。
    // bgfx 版ではまだ中身が無いので、空の型として定義しておく
    //------------------------------------------------------------------------

    //! フルスクリーン三角形の描画（同上）
    class FullscreenPass {};

    //! トーンマップパス（同上）
    class TonemapPass {};

    //! パス単位の GPU タイマー（同上）
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

        m_commandExecutor = std::make_unique<DrawCommandExecutor>();
        if(!m_commandExecutor->Initialize(*m_graphicsContext, m_resources, m_frameConstants, m_drawQueue, shaders.debugVS, shaders.debugPS))
            return false;

        //--------------------------------------------------------------------
        // ビュー。どちらもバックバッファへ、積んだ順のまま描く（bgfx は既定でステートの順に並べ替える）。
        // 深度はリバース Z（手前ほど大きい）なので 0 で消す
        //--------------------------------------------------------------------
        bgfx::setViewMode(BgfxView::World, bgfx::ViewMode::Sequential);
        bgfx::setViewMode(BgfxView::Overlay, bgfx::ViewMode::Sequential);
        bgfx::setViewClear(BgfxView::Overlay, BGFX_CLEAR_DEPTH, 0, 0.0f, 0);

        m_shadowPass = std::make_unique<ShadowPass>();
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

        const u32 rgba = (ToByte(m_clearColor[0]) << 24) | (ToByte(m_clearColor[1]) << 16) | (ToByte(m_clearColor[2]) << 8)
                         | ToByte(m_clearColor[3]);
        const u16 width  = static_cast<u16>(m_graphicsContext->GetWidth());
        const u16 height = static_cast<u16>(m_graphicsContext->GetHeight());
        bgfx::setViewClear(BgfxView::World, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, rgba, 0.0f, 0);
        bgfx::setViewRect(BgfxView::World, 0, 0, width, height);
        bgfx::setViewRect(BgfxView::Overlay, 0, 0, width, height);
        bgfx::touch(BgfxView::World);

        const auto& commands = m_drawQueue.GetCommands();

        //--------------------------------------------------------------------
        // 3D のパス。GBuffer（ディファード）とライティングはまだ無いので、深度を書くフォワード（World）と
        // 半透明（Transparent）だけを、積んだ順にバックバッファへ描く。
        // bgfx 版へまだ移していないシェーダーのコマンドは、DrawCommandExecutor が飛ばす
        //--------------------------------------------------------------------
        m_frameConstants.UploadWorld();
        for(const RenderPass pass : {RenderPass::World, RenderPass::Transparent}) {
            for(const DrawCommand& cmd : commands) {
                if(cmd.pass == pass)
                    m_commandExecutor->Execute(cmd, BgfxView::World, m_frameStats);
            }
        }

        //--------------------------------------------------------------------
        // Overlay パス。DX11 版と同じく sortOrder の昇順（同じ値は積んだ順）に並べて描く。
        // 3D モデル（cameraOverride を持つコマンド）はそのカメラに差し替えて描く。
        // sortOrder が変わるたびに深度を消し直す処理は、モデルを移すとき（段階 1-4 b）に足す
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

        m_frameConstants.UploadOverlay();
        bool overlayCameraOnLoan = false;    // シーン定数のカメラを差し替えているか
        for(u32 index : m_overlayOrder) {
            const DrawCommand& cmd = commands[index];
            if(cmd.cameraOverride) {
                m_frameConstants.UploadWorldWithCamera(*cmd.cameraOverride);
                overlayCameraOnLoan = true;
            } else if(overlayCameraOnLoan) {
                m_frameConstants.UploadOverlay();
                overlayCameraOnLoan = false;
            }
            m_commandExecutor->Execute(cmd, BgfxView::Overlay, m_frameStats);
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

//----------------------------------------------------------------------------
//! @file   GraphicsContext.cpp
//! @brief  bgfx 版の GraphicsContext の実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>

#include <Tsukino/Core/Log.hpp>

#include <bgfx/bgfx.h>

#include <string>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    namespace {
        bool g_bgfxAlive = false;    // bgfx が初期化されていて、まだ後始末されていないか

        //! バックバッファを作り直すときのフラグを返します。
        //! @param  [in] vsync 垂直同期をするか
        //! @return BGFX_RESET_* の組み合わせ
        u32 ResetFlags(bool vsync) {
            return vsync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE;
        }

        //! スワップチェーンの設定を作ります。
        //! @param  [in] windowHandle 描画先のウィンドウ
        //! @param  [in] width        幅
        //! @param  [in] height       高さ
        //! @return スワップチェーンの設定
        //! @note   バックバッファは DX11 版と同じく sRGB にする（書き込むときに linear → sRGB に直す）。
        //!         そうしないと、UI の色が DX11 版より暗く見える。新しい bgfx では
        //!         BGFX_RESET_SRGB_BACKBUFFER ではなく、スワップチェーンのフラグで指定する
        bgfx::SwapChain MakeSwapChain(void* windowHandle, u32 width, u32 height) {
            bgfx::SwapChain swapChain;
            swapChain.nwh    = windowHandle;
            swapChain.width  = width;
            swapChain.height = height;
            swapChain.formatColor = bgfx::TextureFormat::RGBA8;    // DX11 版と同じ R8G8B8A8（書き込みは sRGB のビュー）
            swapChain.flags |= BGFX_SWAP_CHAIN_SRGB_BACKBUFFER;
            return swapChain;
        }
    }    // namespace

    //------------------------------------------------------------------------
    //! bgfx を後始末します。
    //------------------------------------------------------------------------
    GraphicsContext::~GraphicsContext() {
        if(m_initialized) {
            DestroyRenderTargets();
            g_bgfxAlive = false;
            bgfx::shutdown();
        }
    }

    //------------------------------------------------------------------------
    //! bgfx が初期化されていて、まだ後始末されていないかを返します。
    //------------------------------------------------------------------------
    bool GraphicsContext::IsBgfxAlive() noexcept {
        return g_bgfxAlive;
    }

    //------------------------------------------------------------------------
    //! bgfx を初期化します。
    //------------------------------------------------------------------------
    bool GraphicsContext::Initialize(void* windowHandle, u32 width, u32 height, const GraphicsDeviceOptions& /*deviceOptions*/) {
        //--------------------------------------------------------------------
        // 初期化の前に renderFrame() を1回呼ぶと、bgfx は描画用のスレッドを作らず、
        // frame() を呼んだスレッドで描画まで済ませる。エンジンは1本のスレッドで
        // 描くので、こちらに揃える（Web の環境もスレッドを使えないことがある）
        //--------------------------------------------------------------------
        bgfx::renderFrame();

        bgfx::Init init;
#if defined(_WIN32)
        init.type = bgfx::RendererType::Direct3D11;    // Windows は今の DX11 版と同じ API で比べられるようにする
#endif
        init.swapChain        = MakeSwapChain(windowHandle, width, height);
        init.reset            = ResetFlags(m_vsync);

        if(!bgfx::init(init)) {
            Tsukino::Core::Log::Error("GraphicsContext: Failed to initialize bgfx.");
            return false;
        }

        m_initialized  = true;
        g_bgfxAlive    = true;
        m_windowHandle = windowHandle;
        m_width        = width;
        m_height      = height;

        // GPU の情報（名前は bgfx が持っていないので、描画 API の名前で代用する）
        const bgfx::Caps* caps   = bgfx::getCaps();
        m_adapterInfo.name       = std::string("bgfx ") + bgfx::getRendererName(bgfx::getRendererType());
        m_adapterInfo.vendorId   = caps ? caps->vendorId : 0;
        m_adapterInfo.deviceId   = caps ? caps->deviceId : 0;

        Tsukino::Core::Log::Info("GraphicsContext: bgfx initialized (" + m_adapterInfo.name + ").");
        return CreateRenderTargets();
    }

    //------------------------------------------------------------------------
    //! 描画領域の大きさを変えます。
    //------------------------------------------------------------------------
    bool GraphicsContext::Resize(u32 width, u32 height) {
        m_width  = width;
        m_height = height;
        ApplyReset();
        return true;
    }

    //------------------------------------------------------------------------
    //! 垂直同期の有無を設定します。
    //------------------------------------------------------------------------
    void GraphicsContext::SetVSyncEnabled(bool enabled) {
        m_vsync = enabled;
        ApplyReset();
    }

    //------------------------------------------------------------------------
    //! 今の大きさと垂直同期の設定で、bgfx のバックバッファを作り直します。
    //------------------------------------------------------------------------
    void GraphicsContext::ApplyReset() {
        if(!m_initialized || m_width == 0 || m_height == 0)
            return;
        bgfx::SwapChain swapChain = MakeSwapChain(m_windowHandle, m_width, m_height);
        bgfx::reset(ResetFlags(m_vsync), &swapChain);
    }

    //------------------------------------------------------------------------
    //! 3D を描く先のテクスチャとフレームバッファを作ります。
    //------------------------------------------------------------------------
    bool GraphicsContext::CreateRenderTargets() {
        //--------------------------------------------------------------------
        // どれもバックバッファと同じ大きさ（BackbufferRatio::Equal）。bgfx::reset で大きさが変わると、
        // bgfx が作り直してくれる。読むときはどれも点で読むので、サンプラーは点・端で止める
        //--------------------------------------------------------------------
        constexpr u64 kTargetFlags = BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT
                                     | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
        constexpr auto kRatio = bgfx::BackbufferRatio::Equal;

        m_hdrTexture   = bgfx::createTexture2D(kRatio, false, 1, bgfx::TextureFormat::RGBA16F, kTargetFlags);
        m_depthTexture = bgfx::createTexture2D(kRatio, false, 1, bgfx::TextureFormat::D32F, kTargetFlags);

        //--------------------------------------------------------------------
        // G-Buffer。形式は DX11 版と同じ（albedo は sRGB で持つ。ワールド座標は精度のため 32bit）
        //--------------------------------------------------------------------
        struct GBufferFormat {
            bgfx::TextureFormat::Enum format;    // 形式
            u64                       flags;     // 追加のフラグ
        };
        constexpr GBufferFormat kGBufferFormats[kGBufferCount] = {
            {bgfx::TextureFormat::RGBA8,   BGFX_TEXTURE_SRGB},    // 0: albedo
            {bgfx::TextureFormat::RGB10A2, 0},                    // 1: 法線 + ShadingModel
            {bgfx::TextureFormat::RGBA8,   0},                    // 2: 材質
            {bgfx::TextureFormat::RG11B10F, 0},                   // 3: 発光
            {bgfx::TextureFormat::RGBA32F, 0},                    // 4: ワールド座標
            {bgfx::TextureFormat::RG16F,   0},                    // 5: 速度
        };
        for(u32 i = 0; i < kGBufferCount; ++i)
            m_gbufferTextures[i] = bgfx::createTexture2D(kRatio, false, 1, kGBufferFormats[i].format, kTargetFlags | kGBufferFormats[i].flags);

        //--------------------------------------------------------------------
        // フレームバッファ。テクスチャはこのクラスが持ち、フレームバッファと一緒には破棄しない
        //--------------------------------------------------------------------
        bgfx::TextureHandle gbufferAttachments[kGBufferCount + 1];
        for(u32 i = 0; i < kGBufferCount; ++i)
            gbufferAttachments[i] = m_gbufferTextures[i];
        gbufferAttachments[kGBufferCount] = m_depthTexture;
        m_gbufferFrameBuffer = bgfx::createFrameBuffer(kGBufferCount + 1, gbufferAttachments, false);

        const bgfx::TextureHandle hdrAttachments[] = {m_hdrTexture, m_depthTexture};
        m_hdrFrameBuffer      = bgfx::createFrameBuffer(2, hdrAttachments, false);
        m_hdrColorFrameBuffer = bgfx::createFrameBuffer(1, &m_hdrTexture, false);

        if(!bgfx::isValid(m_gbufferFrameBuffer) || !bgfx::isValid(m_hdrFrameBuffer) || !bgfx::isValid(m_hdrColorFrameBuffer)) {
            Tsukino::Core::Log::Error("GraphicsContext: Failed to create the render targets.");
            return false;
        }
        return true;
    }

    //------------------------------------------------------------------------
    //! 3D を描く先を破棄します。
    //------------------------------------------------------------------------
    void GraphicsContext::DestroyRenderTargets() {
        auto destroyFrameBuffer = [](bgfx::FrameBufferHandle& handle) {
            if(bgfx::isValid(handle))
                bgfx::destroy(handle);
            handle = BGFX_INVALID_HANDLE;
        };
        auto destroyTexture = [](bgfx::TextureHandle& handle) {
            if(bgfx::isValid(handle))
                bgfx::destroy(handle);
            handle = BGFX_INVALID_HANDLE;
        };

        destroyFrameBuffer(m_gbufferFrameBuffer);
        destroyFrameBuffer(m_hdrFrameBuffer);
        destroyFrameBuffer(m_hdrColorFrameBuffer);
        destroyTexture(m_hdrTexture);
        destroyTexture(m_depthTexture);
        for(bgfx::TextureHandle& texture : m_gbufferTextures)
            destroyTexture(texture);
    }

}    // namespace Tsukino::Renderer

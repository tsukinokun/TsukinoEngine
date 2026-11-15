//----------------------------------------------------------------------------
//! @file   GraphicsContext.cpp
//! @brief  bgfx 版の GraphicsContext の実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/Bgfx/GraphicsContext.hpp>

#include <Tsukino/Core/Log.hpp>

#include <bgfx/bgfx.h>

#if defined(_WIN32)
    #include <d3d11.h>
    #include <dxgi1_6.h>
    #include <wrl/client.h>
#endif

#include <algorithm>
#include <cmath>
#include <string>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    namespace {
        bool g_bgfxAlive = false;    // bgfx が初期化されていて、まだ後始末されていないか

#if defined(_WIN32)
        // Microsoft Basic Render Driver（WARP）のベンダー ID とデバイス ID（DX11 版と同じ判定）
        constexpr UINT kBasicRenderVendorId = 0x1414;
        constexpr UINT kBasicRenderDeviceId = 0x008C;

        //! ワイド文字列を UTF-8 に直します。
        //! @param  [in] text ワイド文字列（終端あり）
        //! @return UTF-8 の文字列
        std::string ToUtf8(const wchar_t* text) {
            const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
            if(size <= 1)
                return {};
            std::string result(static_cast<size_t>(size - 1), '\0');
            WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), size, nullptr, nullptr);
            return result;
        }

        //! bgfx が作った D3D11 のデバイスから、アダプタの情報を読みます。
        //! @param  [out] info 読めた値を入れる先（読めなかった項目は触らない）
        //! @return true: 読めた
        bool QueryD3D11AdapterInfo(GraphicsAdapterInfo& info) {
            const bgfx::InternalData* data = bgfx::getInternalData();
            if(bgfx::getRendererType() != bgfx::RendererType::Direct3D11 || !data || !data->context)
                return false;

            auto* device = static_cast<ID3D11Device*>(data->context);

            Microsoft::WRL::ComPtr<IDXGIDevice>   dxgiDevice;
            Microsoft::WRL::ComPtr<IDXGIAdapter>  adapter;
            Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter1;
            DXGI_ADAPTER_DESC1                    desc{};
            if(FAILED(device->QueryInterface(IID_PPV_ARGS(dxgiDevice.GetAddressOf()))) || FAILED(dxgiDevice->GetAdapter(adapter.GetAddressOf()))
               || FAILED(adapter.As(&adapter1)) || FAILED(adapter1->GetDesc1(&desc)))
                return false;

            info.name            = ToUtf8(desc.Description);
            info.vendorId        = desc.VendorId;
            info.deviceId        = desc.DeviceId;
            info.dedicatedMemory = desc.DedicatedVideoMemory;
            info.sharedMemory    = desc.SharedSystemMemory;
            info.isSoftware      = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0
                              || (desc.VendorId == kBasicRenderVendorId && desc.DeviceId == kBasicRenderDeviceId);

            // CPU とメモリを共有するかどうか（内蔵 GPU の判定）
            D3D11_FEATURE_DATA_D3D11_OPTIONS2 options2{};
            if(SUCCEEDED(device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &options2, sizeof(options2))))
                info.isUnifiedMemory = options2.UnifiedMemoryArchitecture != FALSE;
            return true;
        }

        //! 高性能を優先した順で、ソフトウェアでない最初のアダプタの ID を探します。
        //! @param  [out] vendorId ベンダー ID
        //! @param  [out] deviceId デバイス ID
        //! @return true: 見つかった
        bool FindHighPerformanceAdapter(u16& vendorId, u16& deviceId) {
            Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
            if(FAILED(CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf()))))
                return false;

            Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
            for(UINT index = 0; SUCCEEDED(factory->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                                              IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())));
                ++index) {
                DXGI_ADAPTER_DESC1 desc{};
                if(SUCCEEDED(adapter->GetDesc1(&desc)) && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
                    vendorId = static_cast<u16>(desc.VendorId);
                    deviceId = static_cast<u16>(desc.DeviceId);
                    return true;
                }
            }
            return false;
        }

        //! bgfx に WARP（ソフトウェア描画）のアダプタを使わせます。
        //! @param  [out] init アダプタの ID を書き込む先
        //! @note   この bgfx の版は BGFX_PCI_ID_SOFTWARE_RASTERIZER を渡しても、合うアダプタが無いので
        //!         既定のアダプタで作ってしまう。WARP も DXGI のアダプタの1つとして列挙されるので、その ID を直接渡す
        void UseSoftwareAdapter(bgfx::Init& init) {
            init.vendorId = static_cast<u16>(kBasicRenderVendorId);
            init.deviceId = static_cast<u16>(kBasicRenderDeviceId);
        }

        //! bgfx に WARP のアダプタを指定しているかを返します。
        //! @param  [in] init 調べる設定
        //! @return true: WARP を指定している
        bool IsSoftwareAdapter(const bgfx::Init& init) {
            return init.vendorId == kBasicRenderVendorId && init.deviceId == kBasicRenderDeviceId;
        }
#endif

        //! デバイスの選び方に合わせて、bgfx に使わせるアダプタを指定します。
        //! @param  [in]  preference デバイスの選び方
        //! @param  [out] init       アダプタの ID を書き込む先
        //! @note   bgfx はベンダー ID とデバイス ID が合う最初のアダプタを使う。指定しなければ OS の既定のアダプタになる
        void SelectAdapter(GraphicsDevicePreference preference, bgfx::Init& init) {
            if(preference == GraphicsDevicePreference::ForceSoftware) {
#if defined(_WIN32)
                UseSoftwareAdapter(init);
#else
                Tsukino::Core::Log::Warn("GraphicsContext: The software rasterizer is not available on this platform. Using the default adapter.");
#endif
                return;
            }
#if defined(_WIN32)
            // 内蔵と外付けの GPU があるノート PC で、外付けの方を使わせる（DX11 版と同じ選び方）
            u16 vendorId = 0;
            u16 deviceId = 0;
            if(FindHighPerformanceAdapter(vendorId, deviceId)) {
                init.vendorId = vendorId;
                init.deviceId = deviceId;
            }
#endif
        }

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
    bool GraphicsContext::Initialize(void* windowHandle, u32 width, u32 height, const GraphicsDeviceOptions& deviceOptions) {
        //--------------------------------------------------------------------
        // 初期化の前に renderFrame() を1回呼ぶと、bgfx は描画用のスレッドを作らず、
        // frame() を呼んだスレッドで描画まで済ませる。エンジンは1本のスレッドで
        // 描くので、こちらに揃える（Web の環境もスレッドを使えないことがある）
        //--------------------------------------------------------------------
        bgfx::renderFrame();

        bgfx::Init init;
#if defined(_WIN32)
        init.type = bgfx::RendererType::Direct3D11;
#endif
        init.swapChain = MakeSwapChain(windowHandle, width, height);
        init.reset     = ResetFlags(m_vsync);
        SelectAdapter(deviceOptions.preference, init);

        bool initialized = bgfx::init(init);
#if defined(_WIN32)
        //--------------------------------------------------------------------
        // ハードウェアで作れなければ、ソフトウェア描画（WARP）で作り直す
        //--------------------------------------------------------------------
        if(!initialized && !IsSoftwareAdapter(init)) {
            Tsukino::Core::Log::Warn("GraphicsContext: No usable hardware device. Falling back to WARP.");
            UseSoftwareAdapter(init);
            initialized = bgfx::init(init);
        }
#endif
        if(!initialized) {
            Tsukino::Core::Log::Error("GraphicsContext: Failed to initialize bgfx.");
            return false;
        }

        m_initialized  = true;
        g_bgfxAlive    = true;
        m_windowHandle = windowHandle;
        m_width        = width;
        m_height      = height;

        Tsukino::Core::Log::Info(std::string("GraphicsContext: bgfx initialized (") + bgfx::getRendererName(bgfx::getRendererType()) + ").");
        QueryAdapterInfo();
        return CreateRenderTargets();
    }

    //------------------------------------------------------------------------
    //! 使っている GPU の情報を調べて保持し、ログへ出します。
    //------------------------------------------------------------------------
    void GraphicsContext::QueryAdapterInfo() {
        m_adapterInfo = GraphicsAdapterInfo{};

#if defined(_WIN32)
        const bool queried = QueryD3D11AdapterInfo(m_adapterInfo);
#else
        const bool queried = false;
#endif
        //--------------------------------------------------------------------
        // 描画 API から読めないときは、bgfx が持つ ID と描画 API の名前で代用する。
        // メモリの量は分からないので 0 のまま（画質の自動判定は低い段階になる）
        //--------------------------------------------------------------------
        if(!queried) {
            const bgfx::Caps* caps = bgfx::getCaps();
            m_adapterInfo.name     = std::string("bgfx ") + bgfx::getRendererName(bgfx::getRendererType());
            m_adapterInfo.vendorId = caps ? caps->vendorId : 0;
            m_adapterInfo.deviceId = caps ? caps->deviceId : 0;
        }

        constexpr u64 kMiB = 1024ull * 1024ull;
        Tsukino::Core::Log::Info("GraphicsContext: Adapter '" + m_adapterInfo.name + "' (software: " + (m_adapterInfo.isSoftware ? "yes" : "no")
                                 + ", unified memory: " + (m_adapterInfo.isUnifiedMemory ? "yes" : "no")
                                 + ", dedicated: " + std::to_string(m_adapterInfo.dedicatedMemory / kMiB)
                                 + " MiB, shared: " + std::to_string(m_adapterInfo.sharedMemory / kMiB) + " MiB)");
    }

    //------------------------------------------------------------------------
    //! 描画領域の大きさを変えます。
    //------------------------------------------------------------------------
    bool GraphicsContext::Resize(u32 width, u32 height) {
        m_width  = width;
        m_height = height;
        ApplyReset();
        return RecreateRenderTargetsIfNeeded();
    }

    //------------------------------------------------------------------------
    //! 内部解像度の倍率を設定し、3D を描く先を作り直します。
    //------------------------------------------------------------------------
    bool GraphicsContext::SetRenderScale(float scale) {
        const float clamped = std::clamp(scale, RenderQualitySettings::kMinRenderScale, 1.0f);
        if(clamped == m_renderScale)
            return true;
        m_renderScale = clamped;
        return RecreateRenderTargetsIfNeeded();
    }

    //------------------------------------------------------------------------
    //! 3D を描く大きさが変わっていれば、描く先を作り直します。
    //------------------------------------------------------------------------
    bool GraphicsContext::RecreateRenderTargetsIfNeeded() {
        // 最小化などで大きさが 0 のときは、作り直さずに今の描く先を残す（DX11 版と同じ）
        if(!m_initialized || m_width == 0 || m_height == 0)
            return true;

        const u32 sceneWidth  = std::max(1u, static_cast<u32>(std::lround(static_cast<float>(m_width) * m_renderScale)));
        const u32 sceneHeight = std::max(1u, static_cast<u32>(std::lround(static_cast<float>(m_height) * m_renderScale)));
        if(sceneWidth == m_sceneWidth && sceneHeight == m_sceneHeight)
            return true;

        DestroyRenderTargets();
        return CreateRenderTargets();
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
        // どれも 3D を描く大きさ（描画領域 × 内部解像度の倍率。等倍ならバックバッファとぴったり同じ）。
        // 既定のサンプラーは点・端で止める（読む側が線形にしたいときは setTexture で指定する）
        //--------------------------------------------------------------------
        constexpr u64 kTargetFlags = BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT
                                     | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
        m_sceneWidth  = std::max(1u, static_cast<u32>(std::lround(static_cast<float>(m_width) * m_renderScale)));
        m_sceneHeight = std::max(1u, static_cast<u32>(std::lround(static_cast<float>(m_height) * m_renderScale)));
        const u16 width  = static_cast<u16>(m_sceneWidth);
        const u16 height = static_cast<u16>(m_sceneHeight);

        m_hdrTexture   = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA16F, kTargetFlags);
        m_depthTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::D32F, kTargetFlags);

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
            m_gbufferTextures[i] = bgfx::createTexture2D(width, height, false, 1, kGBufferFormats[i].format, kTargetFlags | kGBufferFormats[i].flags);

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

        // ポストプロセス用の中間バッファ（HDR と同じ形式。モーションブラーが HDR を読んでここへ書く）
        m_postProcessTexture     = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA16F, kTargetFlags);
        m_postProcessFrameBuffer = bgfx::createFrameBuffer(1, &m_postProcessTexture, false);

        if(!bgfx::isValid(m_gbufferFrameBuffer) || !bgfx::isValid(m_hdrFrameBuffer) || !bgfx::isValid(m_hdrColorFrameBuffer)
           || !bgfx::isValid(m_postProcessFrameBuffer)) {
            Tsukino::Core::Log::Error("GraphicsContext: Failed to create the render targets.");
            return false;
        }

        //--------------------------------------------------------------------
        // エフェクトを描く先（Effekseer を使い始めてから）。トーンマップした絵は 3D の深度と組にし、
        // 歪みの背景とソフトパーティクルの深度は、描く前に写す先（blit）として作る
        //--------------------------------------------------------------------
        if(m_effectTargetsEnabled) {
            constexpr u64 kCopyFlags = BGFX_TEXTURE_BLIT_DST | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
            m_effectColorTexture      = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, kTargetFlags);
            m_effectBackgroundTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, kCopyFlags);
            m_effectDepthTexture      = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::D32F,
                                                              kCopyFlags | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT);
            const bgfx::TextureHandle effectAttachments[] = {m_effectColorTexture, m_depthTexture};
            m_effectFrameBuffer                           = bgfx::createFrameBuffer(2, effectAttachments, false);
            if(!bgfx::isValid(m_effectFrameBuffer) || !bgfx::isValid(m_effectBackgroundTexture) || !bgfx::isValid(m_effectDepthTexture)) {
                Tsukino::Core::Log::Error("GraphicsContext: Failed to create the effect render targets; effects are not drawn.");
                return false;
            }
        }
        return true;
    }

    //------------------------------------------------------------------------
    //! エフェクトを描く先を作るようにします。
    //------------------------------------------------------------------------
    bool GraphicsContext::EnableEffectTargets() {
        if(m_effectTargetsEnabled)
            return true;
        m_effectTargetsEnabled = true;

        // 3D を描く先がまだ無ければ、それを作るときに一緒に作る
        if(m_sceneWidth == 0)
            return true;
        DestroyRenderTargets();
        return CreateRenderTargets();
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
        destroyFrameBuffer(m_postProcessFrameBuffer);
        destroyFrameBuffer(m_effectFrameBuffer);
        destroyTexture(m_effectColorTexture);
        destroyTexture(m_effectBackgroundTexture);
        destroyTexture(m_effectDepthTexture);
        destroyTexture(m_postProcessTexture);
        destroyTexture(m_hdrTexture);
        destroyTexture(m_depthTexture);
        for(bgfx::TextureHandle& texture : m_gbufferTextures)
            destroyTexture(texture);
        m_sceneWidth  = 0;
        m_sceneHeight = 0;
    }

}    // namespace Tsukino::Renderer

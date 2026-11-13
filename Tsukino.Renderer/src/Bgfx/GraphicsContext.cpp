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

    //------------------------------------------------------------------------
    //! bgfx を後始末します。
    //------------------------------------------------------------------------
    GraphicsContext::~GraphicsContext() {
        if(m_initialized)
            bgfx::shutdown();
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
        init.swapChain.nwh    = windowHandle;
        init.swapChain.width  = width;
        init.swapChain.height = height;
        init.reset            = m_vsync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE;

        if(!bgfx::init(init)) {
            Tsukino::Core::Log::Error("GraphicsContext: Failed to initialize bgfx.");
            return false;
        }

        m_initialized  = true;
        m_windowHandle = windowHandle;
        m_width        = width;
        m_height      = height;

        // GPU の情報（名前は bgfx が持っていないので、描画 API の名前で代用する）
        const bgfx::Caps* caps   = bgfx::getCaps();
        m_adapterInfo.name       = std::string("bgfx ") + bgfx::getRendererName(bgfx::getRendererType());
        m_adapterInfo.vendorId   = caps ? caps->vendorId : 0;
        m_adapterInfo.deviceId   = caps ? caps->deviceId : 0;

        Tsukino::Core::Log::Info("GraphicsContext: bgfx initialized (" + m_adapterInfo.name + ").");
        return true;
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
        bgfx::SwapChain swapChain;
        swapChain.nwh    = m_windowHandle;
        swapChain.width  = m_width;
        swapChain.height = m_height;
        bgfx::reset(m_vsync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE, &swapChain);
    }

}    // namespace Tsukino::Renderer

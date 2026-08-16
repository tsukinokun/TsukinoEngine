//--------------------------------------------------------------
//! @file   GraphicsContext.cpp
//! @brief  DirectX11のグラフィックスコンテキストクラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>

#include <Tsukino/Renderer/DX11/PipelineState.hpp>
#include <Tsukino/Renderer/DX11/Material.hpp>

#include <Tsukino/Core/Log.hpp>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @brief 初期化関数
    //--------------------------------------------------------------
    bool GraphicsContext::Initialize(HWND hwnd, UINT width, UINT height) {
        // スワップチェイン設定
        DXGI_SWAP_CHAIN_DESC scDesc{};
        scDesc.BufferCount       = 1;
        scDesc.BufferDesc.Width  = width;
        scDesc.BufferDesc.Height = height;
        scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        scDesc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scDesc.OutputWindow      = hwnd;
        scDesc.SampleDesc.Count  = 1;
        scDesc.Windowed          = TRUE;
        scDesc.SwapEffect        = DXGI_SWAP_EFFECT_DISCARD;

        UINT flags = 0;
#if defined(_DEBUG)
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_0,
        };
        D3D_FEATURE_LEVEL createdLevel{};

        // デバイス・コンテキスト・スワップチェイン作成
        HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr,
                                                   D3D_DRIVER_TYPE_HARDWARE,
                                                   nullptr,
                                                   flags,
                                                   featureLevels,
                                                   _countof(featureLevels),
                                                   D3D11_SDK_VERSION,
                                                   &scDesc,
                                                   m_swapChain.GetAddressOf(),
                                                   m_device.GetAddressOf(),
                                                   &createdLevel,
                                                   m_context.GetAddressOf());

        if(FAILED(hr)) {
            return false;
        }

        //--------------------------------------------------------------
        // 画面サイズに依存するリソース（RTV / DSV / HDRバッファ）を作成する
        //--------------------------------------------------------------
        return CreateSizeDependentResources(width, height);
    }

    //--------------------------------------------------------------
    //! @brief 画面サイズに依存するリソースを解放する
    //--------------------------------------------------------------
    void GraphicsContext::ReleaseSizeDependentResources() {
        //--------------------------------------------------------------
        // ResizeBuffers はバックバッファへの参照が1つでも生きていると
        // DXGI_ERROR_INVALID_CALL で失敗する。
        // ビューを手放したうえで、デバイスコンテキストにバインドされた
        // 状態もクリアしてからでないと参照カウントが落ちない。
        //--------------------------------------------------------------
        if(m_context) {
            ID3D11RenderTargetView* nullRTV[1] = {nullptr};
            m_context->OMSetRenderTargets(1, nullRTV, nullptr);
            m_context->ClearState();
            m_context->Flush();
        }

        m_hdrSRV.Reset();
        m_hdrRTV.Reset();
        m_hdrTex.Reset();
        m_dsv.Reset();
        m_rtv.Reset();
    }

    //--------------------------------------------------------------
    //! @brief 画面サイズに依存するリソースを生成する
    //--------------------------------------------------------------
    bool GraphicsContext::CreateSizeDependentResources(UINT width, UINT height) {
        // バックバッファ取得
        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));

        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("GraphicsContext: Failed to get the swap chain back buffer. Rendering cannot continue.");
            return false;
        }

        // RTV 作成
        hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_rtv.GetAddressOf());

        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("GraphicsContext: Failed to create the back buffer render target view.");
            return false;
        }

        // レンダーターゲット設定
        m_context->OMSetRenderTargets(1, m_rtv.GetAddressOf(), nullptr);

        // ビューポート設定
        D3D11_VIEWPORT vp{};
        vp.TopLeftX = 0.0f;
        vp.TopLeftY = 0.0f;
        vp.Width    = static_cast<float>(width);
        vp.Height   = static_cast<float>(height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;

        m_context->RSSetViewports(1, &vp);

        // Depth buffer texture
        Microsoft::WRL::ComPtr<ID3D11Texture2D> depthTex;

        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width                = width;
        depthDesc.Height               = height;
        depthDesc.MipLevels            = 1;
        depthDesc.ArraySize            = 1;
        depthDesc.Format               = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count     = 1;
        depthDesc.BindFlags            = D3D11_BIND_DEPTH_STENCIL;

        hr = m_device->CreateTexture2D(&depthDesc, nullptr, depthTex.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("GraphicsContext: Failed to create the depth buffer texture. Depth testing would be disabled.");
            return false;
        }

        // DSV 作成
        hr = m_device->CreateDepthStencilView(depthTex.Get(), nullptr, m_dsv.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("GraphicsContext: Failed to create the depth stencil view. Depth testing would be disabled.");
            return false;
        }

        //--------------------------------------------------------------
        // HDRレンダーターゲットの作成
        //--------------------------------------------------------------
        D3D11_TEXTURE2D_DESC hdrDesc{};
        hdrDesc.Width            = width;
        hdrDesc.Height           = height;
        hdrDesc.MipLevels        = 1;
        hdrDesc.ArraySize        = 1;
        hdrDesc.Format           = DXGI_FORMAT_R16G16B16A16_FLOAT;
        hdrDesc.SampleDesc.Count = 1;
        hdrDesc.Usage            = D3D11_USAGE_DEFAULT;
        hdrDesc.BindFlags        = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        hr = m_device->CreateTexture2D(&hdrDesc, nullptr, m_hdrTex.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("GraphicsContext: Failed to create the HDR color buffer.");
            return false;
        }

        hr = m_device->CreateRenderTargetView(m_hdrTex.Get(), nullptr, m_hdrRTV.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("GraphicsContext: Failed to create the HDR render target view.");
            return false;
        }

        hr = m_device->CreateShaderResourceView(m_hdrTex.Get(), nullptr, m_hdrSRV.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("GraphicsContext: Failed to create the HDR shader resource view.");
            return false;
        }

        // 画面サイズを保存
        m_width  = width;
        m_height = height;

        return true;
    }

    //--------------------------------------------------------------
    //! @brief 描画領域のリサイズ
    //--------------------------------------------------------------
    bool GraphicsContext::Resize(UINT width, UINT height) {
        if(!m_swapChain || !m_device || !m_context)
            return false;

        // 最小化などで 0 が来た場合は何もしない（0 サイズのバッファは作れない）
        if(width == 0 || height == 0)
            return true;

        // サイズが変わっていなければ作り直す意味がない
        if(width == m_width && height == m_height)
            return true;

        //--------------------------------------------------------------
        // バックバッファへの参照を全て手放してから ResizeBuffers を呼ぶ
        //--------------------------------------------------------------
        ReleaseSizeDependentResources();

        // 0 を渡すと既存のバッファ数・フォーマットが維持される
        HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("GraphicsContext: IDXGISwapChain::ResizeBuffers failed. The window size change was not applied.");
            return false;
        }

        return CreateSizeDependentResources(width, height);
    }

    //--------------------------------------------------------------
    //! @brief フレームの開始処理
    //--------------------------------------------------------------
    void GraphicsContext::BeginFrame(float r, float g, float b, float a) {
        float clearColor[4] = {r, g, b, a};

        // RenderTarget設定（HDRバッファへ描画）
        m_context->OMSetRenderTargets(1, m_hdrRTV.GetAddressOf(), m_dsv.Get());

        // 画面クリア
        m_context->ClearRenderTargetView(m_hdrRTV.Get(), clearColor);

        // 深度ステンシルクリア
        m_context->ClearDepthStencilView(m_dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);

        // シャドウパスで変更されたビューポートを元に戻す
        D3D11_VIEWPORT vp{};
        vp.TopLeftX = 0.0f;
        vp.TopLeftY = 0.0f;
        vp.Width    = static_cast<float>(m_width);
        vp.Height   = static_cast<float>(m_height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        m_context->RSSetViewports(1, &vp);
    }

    //--------------------------------------------------------------
    //! @brief フレーム終了処理
    //--------------------------------------------------------------
    void GraphicsContext::EndFrame() {
        // SwapChain表示
        m_swapChain->Present(1, 0);
    }

    //--------------------------------------------------------------
    //! @brief  パイプラインステートをセット
    //--------------------------------------------------------------
    void GraphicsContext::SetPipelineState(const PipelineState& state) {
        ID3D11DeviceContext* ctx = m_context.Get();    // コンテキストの生ポインタを取得

        ctx->IASetInputLayout(state.inputLayout.Get());    // 入力レイアウト設定
        ctx->IASetPrimitiveTopology(state.topology);       // プリミティブトポロジー設定

        ctx->VSSetShader(state.vs.Get(), nullptr, 0);    // 頂点シェーダー設定
        ctx->PSSetShader(state.ps.Get(), nullptr, 0);    // ピクセルシェーダー設定

        ctx->RSSetState(state.rasterizer.Get());                         // ラスタライザーステート設定
        ctx->OMSetBlendState(state.blend.Get(), nullptr, 0xffffffff);    // ブレンドステート設定
        ctx->OMSetDepthStencilState(state.depth.Get(), 0);               // デプスステンシルステート設定

        // ブレンドステートを適用（Opaqueのときは nullptr でデフォルトに戻る）
        float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        ctx->OMSetBlendState(state.blend.Get(), blendFactor, 0xFFFFFFFF);
    }

    //--------------------------------------------------------------
    //! @brief  マテリアルをセット
    //--------------------------------------------------------------
    void GraphicsContext::SetMaterial(const Material& mat) {
        SetPipelineState(*mat.GetPipeline());    // マテリアルからパイプラインステートを取得してセット

        ID3D11DeviceContext* ctx = m_context.Get();    // コンテキストの生ポインタを取得

        ID3D11ShaderResourceView* srv = mat.GetTexture();    // マテリアルからテクスチャを取得
        ctx->PSSetShaderResources(0, 1, &srv);               // ピクセルシェーダーのスロット0にテクスチャをセット

        ID3D11SamplerState* sampler = mat.GetSampler();    // マテリアルからサンプラーを取得
        ctx->PSSetSamplers(0, 1, &sampler);                // ピクセルシェーダーのスロット0にサンプラーをセット
    }

    //--------------------------------------------------------------
    //! @brief バックバッファ（スワップチェイン）のRTVにバインドする
    //--------------------------------------------------------------
    void GraphicsContext::BindBackBuffer() {
        // 深度なしでバックバッファにバインド（トーンマッピングは深度不要）
        m_context->OMSetRenderTargets(1, m_rtv.GetAddressOf(), nullptr);
    }
}    // namespace Tsukino::Renderer

//--------------------------------------------------------------
//! @file   GraphicsContext.cpp
//! @brief  DirectX11のグラフィックスコンテキストクラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>

#include <Tsukino/Renderer/DX11/PipelineState.hpp>
#include <Tsukino/Renderer/DX11/Material.hpp>
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
        scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

        // バックバッファ取得
        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));

        if(FAILED(hr)) {
            return false;
        }

        // RTV 作成
        hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_rtv.GetAddressOf());

        if(FAILED(hr)) {
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

        m_device->CreateTexture2D(&depthDesc, nullptr, depthTex.GetAddressOf());

        // DSV 作成
        m_device->CreateDepthStencilView(depthTex.Get(), nullptr, m_dsv.GetAddressOf());

        // 画面サイズを保存
        m_width  = width;
        m_height = height;

        return true;
    }

    //--------------------------------------------------------------
    //! @brief フレームの開始処理
    //--------------------------------------------------------------
    void GraphicsContext::BeginFrame(float r, float g, float b, float a) {
        float clearColor[4] = {r, g, b, a};

        // RenderTarget設定
        m_context->OMSetRenderTargets(1, m_rtv.GetAddressOf(), m_dsv.Get());

        // 画面クリア
        m_context->ClearRenderTargetView(m_rtv.Get(), clearColor);

        // 深度ステンシルクリア
        m_context->ClearDepthStencilView(m_dsv.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

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
}    // namespace Tsukino::Renderer

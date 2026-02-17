//------------------------------------------------------------
//! @file   Renderer.cpp
//! @brief  レンダラークラスの実装
//! @author 山﨑愛
//------------------------------------------------------------
#include "Tsukino/Renderer/Renderer.h"
#include "Tsukino/Renderer/ShaderLoader.h"
#include "Tsukino/Renderer/ConstantBuffer.h"
#include "Tsukino/Core/Log.h"
#include <cassert>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------
    //! @brief レンダラーの初期化
    //------------------------------------------------------------
    bool Renderer::Initialize(HWND hwnd, uint32_t width, uint32_t height) {
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
        ComPtr<ID3D11Texture2D> backBuffer;
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

        //------------------------------------------------------------
        // 三角形描画の準備を追加
        //------------------------------------------------------------
        // 頂点構造体
        struct Vertex {
            float x, y, z;       // 頂点の位置
            float r, g, b, a;    // 頂点の色
        };

        // 三角形の頂点データ
        Vertex vertices[] = {
            {0.0f,  0.5f,  0.0f, 1, 0, 0, 1}, // 上（赤）
            {0.5f,  -0.5f, 0.0f, 0, 1, 0, 1}, // 右（緑）
            {-0.5f, -0.5f, 0.0f, 0, 0, 1, 1}, // 左（青）
        };

        // 頂点バッファの作成
        D3D11_BUFFER_DESC bd{};                                                                      // バッファの説明
        bd.Usage     = D3D11_USAGE_DEFAULT;                                                          // 使用方法
        bd.ByteWidth = sizeof(vertices);                                                             // バッファのサイズ
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;                                                     // 頂点バッファとして使用
        D3D11_SUBRESOURCE_DATA initData{};                                                           // 初期データ
        initData.pSysMem = vertices;                                                                 // 頂点データのポインタ
        hr               = m_device->CreateBuffer(&bd, &initData, m_vertexBuffer.GetAddressOf());    // バッファの作成

        if(FAILED(hr))
            return false;

        // シェーダーブロブとエラーブロブ
        ComPtr<ID3DBlob> vsBlob;       // 頂点シェーダーブロブ
        ComPtr<ID3DBlob> psBlob;       // ピクセルシェーダーブロブ
        ComPtr<ID3DBlob> errorBlob;    // エラーブロブ

        // 頂点シェーダーをコンパイル
        hr = D3DCompileFromFile(L"../../Tsukino.Renderer/include/Tsukino/Renderer/Shaders/TriangleVS.hlsl",
                                nullptr,
                                nullptr,
                                "VSMain",
                                "vs_5_0",
                                0,
                                0,
                                vsBlob.GetAddressOf(),
                                errorBlob.GetAddressOf());
        // コンパイル失敗時
        if(FAILED(hr)) {
            if(errorBlob) {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
            }
            assert(false && "VS compile failed: check file path or syntax");
        }
        // 頂点シェーダー作成
        hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, m_vertexShader.GetAddressOf());
        assert(SUCCEEDED(hr));

        // ピクセルシェーダー
        hr = D3DCompileFromFile(L"../../Tsukino.Renderer/include/Tsukino/Renderer/Shaders/TrianglePS.hlsl",
                                nullptr,
                                nullptr,
                                "PSMain",
                                "ps_5_0",
                                0,
                                0,
                                psBlob.GetAddressOf(),
                                errorBlob.GetAddressOf());
        // コンパイル失敗時
        if(FAILED(hr)) {
            if(errorBlob) {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
            }
            assert(false && "PS compile failed: check file path or syntax");
        }
        // ピクセルシェーダー作成
        hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, m_pixelShader.GetAddressOf());
        assert(SUCCEEDED(hr));

        // 入力レイアウト（POSITION: float3, COLOR: float4）
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                       D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, UINT(3 * sizeof(float)), D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        // 入力レイアウト作成
        hr = m_device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), m_inputLayout.GetAddressOf());
        assert(SUCCEEDED(hr));

        //------------------------------------------------------------
        // 定数バッファの作成
        //------------------------------------------------------------
        if(!CreateConstantBuffer())
            return false;    // 定数バッファの作成に失敗した場合は false を返す

        return true;
    }

    //------------------------------------------------------------
    //! @brief 定数バッファの作成
    //------------------------------------------------------------
    bool Renderer::CreateConstantBuffer() {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth         = sizeof(Tsukino::Renderer::CBufferTransform);
        desc.Usage             = D3D11_USAGE_DEFAULT;
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;

        HRESULT hr = m_device->CreateBuffer(&desc, nullptr, m_constantBuffer.GetAddressOf());
        // 定数バッファの作成に失敗した場合はエラーログを出力
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create constant buffer.");
            return false;
        }
        return true;
    }

    //------------------------------------------------------------
    //! @brief 描画処理
    //------------------------------------------------------------
    void Renderer::Render() {
        // 設定されたクリアカラーで画面をクリア
        m_context->ClearRenderTargetView(m_rtv.Get(), m_clearColor.data());

        UINT stride = sizeof(float) * 7;    // Vertex のサイズ
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_context->IASetInputLayout(m_inputLayout.Get());
        m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

        // シェーダと入力レイアウトをセット（後で追加）
        m_context->Draw(3, 0);

        // 表示
        m_swapChain->Present(1, 0);
    }

    //------------------------------------------------------------
    //! @brief クリアカラー設定
    //------------------------------------------------------------
    void Renderer::SetClearColor(float r, float g, float b, float a) {
        m_clearColor = {r, g, b, a};
    }
}    // namespace Tsukino::Renderer

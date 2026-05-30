//------------------------------------------------------------
//! @file   Renderer.cpp
//! @brief  レンダラークラスの実装
//! @author 山﨑愛
//------------------------------------------------------------
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/ShaderLoader.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>

#include <Tsukino/GraphicsCommon/Mesh/MeshPrimitives.hpp>

#include <Tsukino/Core/Log.hpp>

#include <cassert>
#include <d3dcompiler.h>
#include <algorithm>

#pragma comment(lib, "d3dcompiler.lib")

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------
    //! @brief レンダラーの初期化
    //------------------------------------------------------------
    bool Renderer::Initialize(HWND hwnd, uint32_t width, uint32_t height) {
        // グラフィックスコンテキストの初期化
        if(!m_graphicsContext.Initialize(hwnd, width, height)) {
            return false;
        }

        ID3D11Device*        device  = m_graphicsContext.GetDevice();     // DirectXのDevice
        ID3D11DeviceContext* context = m_graphicsContext.GetContext();    // DirectXのDeviceContext

        // デバイスが準備できたので、Factoryを構築してoptionalに代入（遅延DI）
        m_pipelineFactory.emplace(device);

        //------------------------------------------------------------
        // DirectXTKの共通ステートの作成
        //------------------------------------------------------------
        m_commonStatesTK = std::make_unique<DirectX::CommonStates>(device);    // DirectXTKの共通ステートを作成

        //------------------------------------------------------------
        // メッシュバッファの作成
        //------------------------------------------------------------
        if(!CreatePrimitiveMeshes())
            return false;    // メッシュバッファの作成に失敗した場合は false を返す

        //------------------------------------------------------------
        // 共通ステートの作成
        //------------------------------------------------------------
        if(!CreateCommonStates())
            return false;    // 共通ステートの作成に失敗した場合は false を返す

        //------------------------------------------------------------
        // 定数バッファの作成
        //------------------------------------------------------------
        if(!CreateConstantBuffer())
            return false;    // 定数バッファの作成に失敗した場合は false を返す

        //------------------------------------------------------------
        // デバッグ用バッファの作成
        //------------------------------------------------------------
        if(!CreateDebugBuffers())
            return false;

        return true;
    }

    //------------------------------------------------------------
    //! @brief 定数バッファの作成
    //------------------------------------------------------------
    bool Renderer::CreateConstantBuffer() {
        //デバイスを取得
        ID3D11Device* device = m_graphicsContext.GetDevice();

        D3D11_BUFFER_DESC desc = {};
        desc.Usage             = D3D11_USAGE_DEFAULT;
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;

        //------------------------------------------------------------
        // m_sceneBuffer (b0) の作成
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferScene);
        HRESULT hr     = device->CreateBuffer(&desc, nullptr, m_sceneBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create scene constant buffer.");
            return false;
        }

        //------------------------------------------------------------
        // m_objectBuffer (b1) の作成
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferTransform);
        hr             = device->CreateBuffer(&desc, nullptr, m_objectBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create object constant buffer.");
            return false;
        }

        //------------------------------------------------------------
        // m_materialBuffer (b2) の作成
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferMaterial);
        hr             = device->CreateBuffer(&desc, nullptr, m_materialBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create material constant buffer.");
            return false;
        }

        // ------------------------------------------------------------
        // m_skinningBuffer (b3) の作成
        // ------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferSkinning);
        hr             = device->CreateBuffer(&desc, nullptr, m_skinningBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create skinning constant buffer.");
            return false;
        }

        // 成功
        return true;
    }

    //------------------------------------------------------------
    //! @brief デバッグ用バッファの作成
    //------------------------------------------------------------
    bool Renderer::CreateDebugBuffers() {
        ID3D11Device* device = m_graphicsContext.GetDevice();

        // 頂点シェーダのコンパイルと読み込み
        Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT                          hr = D3DCompileFromFile(
            L"Tsukino.BuiltIn/Assets/Shaders/DebugLine.vs.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &vsBlob, &errorBlob);
        if(FAILED(hr)) {
            if(errorBlob)
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            return false;
        }
        device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, m_debugVS.GetAddressOf());

        // ピクセルシェーダのコンパイルと読み込み
        Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
        hr = D3DCompileFromFile(
            L"Tsukino.BuiltIn/Assets/Shaders/DebugLine.ps.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &psBlob, &errorBlob);
        if(FAILED(hr)) {
            if(errorBlob)
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            return false;
        }
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, m_debugPS.GetAddressOf());

        // 入力レイアウトの作成
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Tsukino::GraphicsCommon::DebugVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Tsukino::GraphicsCommon::DebugVertex, color),    D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), m_debugIL.GetAddressOf());

        // 動的頂点バッファの作成
        D3D11_BUFFER_DESC bd = {};
        bd.Usage             = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth         = sizeof(Tsukino::GraphicsCommon::DebugVertex) * 50000;    // 最大 50,000 頂点
        bd.BindFlags         = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

        device->CreateBuffer(&bd, nullptr, m_debugLineVB.GetAddressOf());
        device->CreateBuffer(&bd, nullptr, m_debugTriangleVB.GetAddressOf());

        return true;
    }

    //------------------------------------------------------------
    //! @brief プリミティブメッシュの作成
    //------------------------------------------------------------
    bool Renderer::CreatePrimitiveMeshes() {
        using namespace Tsukino::GraphicsCommon;

        for(size_t i = 0; i < (size_t)PrimitiveType::Count; ++i) {
            PrimitiveType type = static_cast<PrimitiveType>(i);

            // CPU 側で形状生成
            MeshData meshData = Tsukino::GraphicsCommon::CreatePrimitiveMeshData(type);

            // GPU にアップロード
            m_primitiveMeshes[i] = CreateMeshBuffer(m_graphicsContext.GetDevice(), meshData);
        }

        return true;
    }

    //------------------------------------------------------------
    //! @brief 描画処理
    //------------------------------------------------------------
    void Renderer::Render() {
        // 設定されたクリアカラーで画面をクリア
        m_graphicsContext.BeginFrame(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);

        //------------------------------------------------------------
        // 描画コマンドの実行
        //------------------------------------------------------------
        const auto& commands = m_drawQueue.GetCommands();

        //------------------------------------------------------------
        // World パス
        //------------------------------------------------------------
        UpdateSceneBuffer(m_worldSceneData);
        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::World)
                continue;
            ExecuteDrawCommand(cmd);
        }

        //------------------------------------------------------------
        // Overlayパス
        //------------------------------------------------------------
        UpdateSceneBuffer(m_overlaySceneData);
        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::Overlay)
                continue;
            ExecuteDrawCommand(cmd);
        }

        // 描画コマンドのクリア
        m_drawQueue.Clear();

        // 表示
        m_graphicsContext.EndFrame();
    }

    //------------------------------------------------------------
    //! @brief デバッグ描画の実行
    //------------------------------------------------------------
    void Renderer::FlushDebugDraw() {
        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        UpdateSceneBuffer(m_worldSceneData);

        if(!m_debugLineVertices.empty() || !m_debugTriangleVertices.empty()) {
            context->VSSetShader(m_debugVS.Get(), nullptr, 0);
            context->PSSetShader(m_debugPS.Get(), nullptr, 0);
            context->IASetInputLayout(m_debugIL.Get());

            // ブレンドステート（不透明）と深度有効を設定（適宜CommonStates等で）
            context->OMSetBlendState(m_commonStatesTK->Opaque(), nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(m_commonStatesTK->DepthDefault(), 0);
            // レンダリングステート設定（ここでは必要に応じてワイヤーフレーム用等の設定が必要になる可能性）
            context->RSSetState(m_commonStatesTK->CullNone());

            UINT stride = sizeof(Tsukino::GraphicsCommon::DebugVertex);
            UINT offset = 0;

            // --- ラインの描画 ---
            if(!m_debugLineVertices.empty()) {
                D3D11_MAPPED_SUBRESOURCE mapped;
                if(SUCCEEDED(context->Map(m_debugLineVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                    size_t count = std::min(m_debugLineVertices.size(), (size_t)50000);
                    memcpy(mapped.pData, m_debugLineVertices.data(), count * stride);
                    context->Unmap(m_debugLineVB.Get(), 0);

                    context->IASetVertexBuffers(0, 1, m_debugLineVB.GetAddressOf(), &stride, &offset);
                    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
                    context->Draw((UINT)count, 0);
                }
                m_debugLineVertices.clear();
            }

            // --- 三角形の描画 ---
            if(!m_debugTriangleVertices.empty()) {
                D3D11_MAPPED_SUBRESOURCE mapped;
                if(SUCCEEDED(context->Map(m_debugTriangleVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                    size_t count = std::min(m_debugTriangleVertices.size(), (size_t)50000);
                    memcpy(mapped.pData, m_debugTriangleVertices.data(), count * stride);
                    context->Unmap(m_debugTriangleVB.Get(), 0);

                    context->IASetVertexBuffers(0, 1, m_debugTriangleVB.GetAddressOf(), &stride, &offset);
                    context->RSSetState(m_commonStatesTK->Wireframe());    // 三角形はワイヤーフレームで描画
                    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    context->Draw((UINT)count, 0);
                    context->RSSetState(m_commonStatesTK->CullNone());    // 元に戻す
                }
                m_debugTriangleVertices.clear();
            }
        }
    }

    //------------------------------------------------------------
    //! @brief クリアカラー設定
    //------------------------------------------------------------
    void Renderer::SetClearColor(float r, float g, float b, float a) {
        m_clearColor = {r, g, b, a};
    }

    //------------------------------------------------------------
    //! @brief デバッグラインの追加
    //------------------------------------------------------------
    void Renderer::DrawDebugLine(const Tsukino::GraphicsCommon::DebugVertex& v1, const Tsukino::GraphicsCommon::DebugVertex& v2) {
        m_debugLineVertices.push_back(v1);
        m_debugLineVertices.push_back(v2);
    }

    //------------------------------------------------------------
    //! @brief デバッグ三角形の追加
    //------------------------------------------------------------
    void Renderer::DrawDebugTriangle(const Tsukino::GraphicsCommon::DebugVertex& v1,
                                     const Tsukino::GraphicsCommon::DebugVertex& v2,
                                     const Tsukino::GraphicsCommon::DebugVertex& v3) {
        m_debugTriangleVertices.push_back(v1);
        m_debugTriangleVertices.push_back(v2);
        m_debugTriangleVertices.push_back(v3);
    }

    //------------------------------------------------------------
    //! @brief 描画コマンドの追加
    //------------------------------------------------------------
    void Renderer::PushDrawCommand(const DrawCommand& cmd) {
        m_drawQueue.Push(cmd);
    }

    //------------------------------------------------------------
    //! @brief テクスチャ（SRV）の取得（なければ生成してキャッシュ）
    //------------------------------------------------------------
    ID3D11ShaderResourceView* Renderer::GetTextureSRV(const Tsukino::Asset::TextureAsset& textureAsset) {
        uint64_t handleValue = textureAsset.GetHandle().Value();

        // 1. すでにキャッシュにあるか探す
        auto it = m_textureCache.find(handleValue);
        if(it != m_textureCache.end()) {
            return it->second->GetSRV();    // 既存のものを返す
        }

        // 2. なければ新しく作成する
        ID3D11Device* device = m_graphicsContext.GetDevice();

        std::unique_ptr<DX11Texture2D> texture =
            std::make_unique<DX11Texture2D>(textureAsset.width, textureAsset.height, textureAsset.format, textureAsset.pixels.data(), device);

        ID3D11ShaderResourceView* srv = texture->GetSRV();

        // 3. キャッシュに保存
        m_textureCache.emplace(handleValue, std::move(texture));

        return srv;
    }

    //------------------------------------------------------------
    //! @brief シーン定数バッファの更新
    //------------------------------------------------------------
    void Renderer::UpdateSceneBuffer(const CBufferScene& sceneData) {
        // デバイスコンテキストを取得
        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        //------------------------------------------------------------
        // GPU上のバッファ（m_sceneBuffer）の中身を書き換える
        //------------------------------------------------------------
        context->UpdateSubresource(m_sceneBuffer.Get(), 0, nullptr, &sceneData, 0, 0);

        //------------------------------------------------------------
        // スロット0（b0）にバインドする
        //------------------------------------------------------------
        context->VSSetConstantBuffers(0, 1, m_sceneBuffer.GetAddressOf());
    }

    //------------------------------------------------------------
    //! @brief SpriteFontの作成
    //------------------------------------------------------------
    std::unique_ptr<DirectX::SpriteFont> Renderer::CreateSpriteFont(const u8* data, size_t size) {
        // ここで DirectX 11 のデバイスを使って、バイナリを「文字」として魂を吹き込む
        return std::make_unique<DirectX::SpriteFont>(m_graphicsContext.GetDevice(), data, size);
    }

    //------------------------------------------------------------
    //! @brief ワールドカメラ行列のセット
    //------------------------------------------------------------
    void Renderer::SetWorldCameraMatrix(const CBufferScene& data) {
        m_worldSceneData = data;    // メンバ変数に保存
    }

    //------------------------------------------------------------
    //! @brief オーバーレイカメラ行列のセット
    //------------------------------------------------------------
    void Renderer::SetOverlayCameraMatrix(const CBufferScene& data) {
        m_overlaySceneData = data;    // メンバ変数に保存
    }

    //------------------------------------------------------------
    //! @brief SpriteBatchの作成
    //------------------------------------------------------------
    std::unique_ptr<DirectX::SpriteBatch> Renderer::CreateSpriteBatch() {
        // Rendererが持っている m_deviceContext (ID3D11DeviceContext*) を渡す
        // ※内部で ComPtr を使っている場合は .Get() で生ポインタを渡します
        return std::make_unique<DirectX::SpriteBatch>(m_graphicsContext.GetContext());
    }

    //------------------------------------------------------------
    //! @brief 描画コマンドの実行
    //------------------------------------------------------------
    void Renderer::ExecuteDrawCommand(const DrawCommand& cmd) {
        // デバイスコンテキストを取得
        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        //------------------------------------------------------------
        // カスタム描画（フォント等）がある場合
        //------------------------------------------------------------
        if(cmd.customDraw) {
            // 1. スロットをクリア
            ID3D11Buffer* nullBuffers[] = {nullptr, nullptr};
            UINT          strides[]     = {0, 0};
            UINT          offsets[]     = {0, 0};
            context->IASetVertexBuffers(0, 2, nullBuffers, strides, offsets);

            // 2. カスタム描画実行
            cmd.customDraw(context);

            // 3. 重要：SpriteBatchで汚されたステートをリセット
            // これを入れないとSpriteの後の描画が真っ暗になったり崩れます
            context->OMSetBlendState(m_commonStatesTK->Opaque(), nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(m_commonStatesTK->DepthDefault(), 0);
            context->RSSetState(m_commonStatesTK->CullNone());

            return;
        }

        //------------------------------------------------------------
        // 無効なコマンドは何もしない
        //------------------------------------------------------------
        if(!cmd.material || !cmd.mesh)
            return;

        //------------------------------------------------------
        // Scene (b0) を毎回再バインド（ステート汚染対策）
        //------------------------------------------------------
        context->VSSetConstantBuffers(0, 1, m_sceneBuffer.GetAddressOf());

        //------------------------------------------------------------
        // 通常描画
        //------------------------------------------------------------

        //------------------------------------------------------
        // Transform を定数バッファに書き込む
        //------------------------------------------------------
        CBufferTransform cb{};
        cb.world = cmd.transform;
        context->UpdateSubresource(m_objectBuffer.Get(), 0, nullptr, &cb, 0, 0);
        context->VSSetConstantBuffers(1, 1, m_objectBuffer.GetAddressOf());

        // ------------------------------------------------------------
        // ボーン行列 (b3) の適用
        // ------------------------------------------------------------
        if(cmd.boneMatrices && cmd.boneCount > 0) {
            CBufferSkinning cbSkin{};
            uint32_t        copyCount = std::min(cmd.boneCount, 128u);
            std::memcpy(cbSkin.bones, cmd.boneMatrices, sizeof(hlslpp::float4x4) * copyCount);

            context->UpdateSubresource(m_skinningBuffer.Get(), 0, nullptr, &cbSkin, 0, 0);
            context->VSSetConstantBuffers(3, 1, m_skinningBuffer.GetAddressOf());
        } else {
            // スキニングを使わないオブジェクトを描画するときは、
            // スロット3を nullptr でクリアして、前のオブジェクトのボーン行列が残らないようにする
            ID3D11Buffer* nullBuffer = nullptr;
            context->VSSetConstantBuffers(3, 1, &nullBuffer);
        }

        //------------------------------------------------------
        // Material を適用
        //------------------------------------------------------
        m_graphicsContext.SetMaterial(*cmd.material);

        if(cmd.materialData) {
            context->UpdateSubresource(m_materialBuffer.Get(), 0, nullptr, cmd.materialData, 0, 0);
            context->PSSetConstantBuffers(2, 1, m_materialBuffer.GetAddressOf());
        }

        //------------------------------------------------------
        // MeshBuffer をセット
        //------------------------------------------------------
        UINT          stride = cmd.mesh->stride;
        UINT          offset = 0;
        ID3D11Buffer* vb     = cmd.mesh->vertexBuffer.Get();

        context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        if(cmd.boneMatrices && cmd.boneCount > 0 && cmd.mesh->boneWeightBuffer.Get() != nullptr) {
            // ボーンあり：スロット0と1をバインド
            ID3D11Buffer* vbs[]     = {cmd.mesh->vertexBuffer.Get(), cmd.mesh->boneWeightBuffer.Get()};
            UINT          strides[] = {cmd.mesh->stride, sizeof(Tsukino::GraphicsCommon::BoneWeight)};
            UINT          offsets[] = {0, 0};
            context->IASetVertexBuffers(0, 2, vbs, strides, offsets);
        } else {
            // ボーンなし：スロット0のみバインドし、スロット1は必ず明示的にクリア！
            UINT          stride = cmd.mesh->stride;
            UINT          offset = 0;
            ID3D11Buffer* vb     = cmd.mesh->vertexBuffer.Get();

            // スロット0に頂点バッファ、スロット1にNULLをセットしてクリアする
            ID3D11Buffer* vbs[]     = {vb, nullptr};
            UINT          strides[] = {stride, 0};
            UINT          offsets[] = {0, 0};
            context->IASetVertexBuffers(0, 2, vbs, strides, offsets);
        }

        context->IASetIndexBuffer(cmd.mesh->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        //------------------------------------------------------
        // 描画
        //------------------------------------------------------
        context->DrawIndexed(cmd.mesh->indexCount, 0, 0);
    }

    //------------------------------------------------------------
    //! @brief 共通ステート（サンプラー等）の作成
    //------------------------------------------------------------
    bool Renderer::CreateCommonStates() {
        // デバイスを取得
        ID3D11Device*      device = m_graphicsContext.GetDevice();
        HRESULT            hr;
        D3D11_SAMPLER_DESC desc{};

        // 共通設定
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.MinLOD         = 0;
        desc.MaxLOD         = D3D11_FLOAT32_MAX;

        // --- PointWrap ---
        desc.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        hr            = device->CreateSamplerState(&desc, m_samplers[static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::PointWrap)].GetAddressOf());
        if(FAILED(hr))
            return false;

        // --- PointClamp ---
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        hr            = device->CreateSamplerState(&desc, m_samplers[static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::PointClamp)].GetAddressOf());
        if(FAILED(hr))
            return false;

        // --- LinearWrap ---
        desc.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        hr            = device->CreateSamplerState(&desc, m_samplers[static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::LinearWrap)].GetAddressOf());
        if(FAILED(hr))
            return false;

        // --- LinearClamp ---
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        hr            = device->CreateSamplerState(&desc, m_samplers[static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::LinearClamp)].GetAddressOf());
        if(FAILED(hr))
            return false;

        // --- AnisotropicWrap ---
        desc.Filter        = D3D11_FILTER_ANISOTROPIC;
        desc.AddressU      = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV      = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW      = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MaxAnisotropy = 16;
        hr = device->CreateSamplerState(&desc, m_samplers[static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::AnisotropicWrap)].GetAddressOf());
        if(FAILED(hr))
            return false;

        // --- AnisotropicClamp ---
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        hr = device->CreateSamplerState(&desc, m_samplers[static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::AnisotropicClamp)].GetAddressOf());
        if(FAILED(hr))
            return false;

        return true;
    }
}    // namespace Tsukino::Renderer

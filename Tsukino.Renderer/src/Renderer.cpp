//------------------------------------------------------------
//! @file   Renderer.cpp
//! @brief  レンダラークラスの実装
//! @author 山﨑愛
//------------------------------------------------------------
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Renderer/ShaderLoader.hpp>
#include <Tsukino/Renderer/ConstantBuffer.hpp>
#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

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
    bool Renderer::Initialize(HWND                               hwnd,
                              uint32_t                           width,
                              uint32_t                           height,
                              const Tsukino::Asset::ShaderAsset* debugVS,
                              const Tsukino::Asset::ShaderAsset* debugPS,
                              const Tsukino::Asset::ShaderAsset* tonemapVS,
                              const Tsukino::Asset::ShaderAsset* tonemapPS,
                              const Tsukino::Asset::ShaderAsset* shadowStaticVS,
                              const Tsukino::Asset::ShaderAsset* shadowSkeletalVS,
                              const Tsukino::Asset::ShaderAsset* shadowPS) {
        // グラフィックスコンテキストの初期化
        if(!m_graphicsContext.Initialize(hwnd, width, height)) {
            return false;
        }

        Tsukino::Core::Log::Info("sizeof(hlslpp::float4x4) = " + std::to_string(sizeof(hlslpp::float4x4)));
        Tsukino::Core::Log::Info("sizeof(CBufferSkinning) = " + std::to_string(sizeof(CBufferSkinning)));

        ID3D11Device*        device  = m_graphicsContext.GetDevice();     // DirectXのDevice
        ID3D11DeviceContext* context = m_graphicsContext.GetContext();    // DirectXのDeviceContext

        // デバイスが準備できたので、Factoryを構築してoptionalに代入（遅延DI）
        m_pipelineFactory.emplace(device);

        //------------------------------------------------------------
        // DirectXTKの共通ステートの作成
        //------------------------------------------------------------
        m_commonStatesTK = std::make_unique<DirectX::CommonStates>(device);    // DirectXTKの共通ステートを作成

        //------------------------------------------------------------
        // シャドウパイプラインの生成
        // ------------------------------------------------------------
        if(!CreateShadowPipelines(shadowStaticVS, shadowSkeletalVS, shadowPS)) {
            Tsukino::Core::Log::Error("Failed to create shadow pipelines.");
            return false;
        }

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
        // 白テクスチャの作成
        //------------------------------------------------------------
        if(!CreateWhiteTexture())
            return false;    // 白テクスチャの作成に失敗した場合は false を返す

        //------------------------------------------------------------
        // デバッグ用バッファの作成
        //------------------------------------------------------------
        if(!CreateDebugBuffers(debugVS, debugPS))
            return false;

        //------------------------------------------------------------
        // トーンマッピングパイプラインの作成
        //------------------------------------------------------------
        SetTonemapPipeline(tonemapVS, tonemapPS);

        //------------------------------------------------------------
        // シャドウマップ用リソースの作成
        //------------------------------------------------------------
        if(!CreateShadowMap())
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

        //------------------------------------------------------------
        // m_skyBuffer (b4) の作成
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferSky);
        hr             = device->CreateBuffer(&desc, nullptr, m_skyBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create sky constant buffer.");
            return false;
        }

        // 成功
        return true;
    }

    //------------------------------------------------------------
    //! @brief デバッグ用バッファの作成
    //------------------------------------------------------------
    bool Renderer::CreateDebugBuffers(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps) {
        if(!vs || !ps) {
            Tsukino::Core::Log::Error("Debug shader assets are null.");
            return false;
        }

        ID3D11Device* device = m_graphicsContext.GetDevice();

        // 頂点シェーダーの作成
        HRESULT hr = device->CreateVertexShader(vs->binary.data(), vs->binary.size(), nullptr, m_debugVS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create debug vertex shader.");
            return false;
        }

        // ピクセルシェーダーの作成
        hr = device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_debugPS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create debug pixel shader.");
            return false;
        }

        // 入力レイアウトの作成
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Tsukino::GraphicsCommon::DebugVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Tsukino::GraphicsCommon::DebugVertex, color),    D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), vs->binary.data(), vs->binary.size(), m_debugIL.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create debug input layout.");
            return false;
        }

        // 動的頂点バッファの作成
        D3D11_BUFFER_DESC bd{};
        bd.Usage          = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth      = sizeof(Tsukino::GraphicsCommon::DebugVertex) * 50000;
        bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = device->CreateBuffer(&bd, nullptr, m_debugLineVB.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create debug line vertex buffer.");
            return false;
        }

        hr = device->CreateBuffer(&bd, nullptr, m_debugTriangleVB.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create debug triangle vertex buffer.");
            return false;
        }

        return true;
    }

    //------------------------------------------------------------
    //! @brief シャドウマップ用パイプラインの作成
    //------------------------------------------------------------
    bool Renderer::CreateShadowPipelines(const Tsukino::Asset::ShaderAsset* shadowStaticVS,
                                         const Tsukino::Asset::ShaderAsset* shadowSkeletalVS,
                                         const Tsukino::Asset::ShaderAsset* shadowPS) {
        auto* factory = GetPipelineFactory();
        if(!factory)
            return false;

        // 静的メッシュ用シャドウパイプライン
        if(shadowStaticVS && shadowPS) {
            m_shadowStaticPipeline = factory->Create(*shadowStaticVS, *shadowPS, Tsukino::GraphicsCommon::VertexFormat::PositionNormalUV, DepthMode::ReadWrite);
            if(!m_shadowStaticPipeline) {
                Tsukino::Core::Log::Error("Renderer: Shadow Static Pipeline generation failed.");
                return false;
            }
        }

        // スキニングメッシュ用シャドウパイプライン
        if(shadowSkeletalVS && shadowPS) {
            m_shadowSkeletalPipeline = factory->Create(*shadowSkeletalVS, *shadowPS, Tsukino::GraphicsCommon::VertexFormat::Skinned, DepthMode::ReadWrite);
            if(!m_shadowSkeletalPipeline) {
                Tsukino::Core::Log::Error("Renderer: Shadow Skeletal Pipeline generation failed.");
                return false;
            }
        }

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
        m_graphicsContext.BeginFrame(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);

        const auto& commands = m_drawQueue.GetCommands();

        //------------------------------------------------------------
        // Shadow パス
        //------------------------------------------------------------
        if(m_shadowStaticPipeline || m_shadowSkeletalPipeline) {
            ID3D11DeviceContext* context = m_graphicsContext.GetContext();

            // シャドウマップをクリア
            context->ClearDepthStencilView(m_shadowMapDSV.Get(), D3D11_CLEAR_DEPTH, 0.0f, 0);

            // カラーRTをnullにしてDSVだけセット
            ID3D11RenderTargetView* nullRTV = nullptr;
            context->OMSetRenderTargets(1, &nullRTV, m_shadowMapDSV.Get());

            // シャドウマップ解像度でビューポートをセット
            D3D11_VIEWPORT vp{};
            vp.Width    = static_cast<float>(SHADOW_MAP_SIZE);
            vp.Height   = static_cast<float>(SHADOW_MAP_SIZE);
            vp.MaxDepth = 1.0f;
            context->RSSetViewports(1, &vp);

            UpdateSceneBuffer(m_worldSceneData);

            for(const auto& cmd : commands) {
                if(cmd.pass != RenderPass::World)
                    continue;
                ExecuteShadowCommand(cmd);
            }

            // RTとビューポートをBeginFrame時の状態に戻す
            m_graphicsContext.BeginFrame(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
        }

        //------------------------------------------------------------
        // Sky パス（Worldパスの前、深度書き込みなし）
        //------------------------------------------------------------
        UpdateSceneBuffer(m_worldSceneData);
        ExecuteSkyPass();

        //------------------------------------------------------------
        // World パス
        //------------------------------------------------------------
        {
            ID3D11DeviceContext* context = m_graphicsContext.GetContext();
            // シャドウマップをt1・s1にバインド
            constexpr UINT shadowSRVSlot     = static_cast<UINT>(SRVSlot::ShadowMap);
            constexpr UINT shadowSamplerSlot = static_cast<UINT>(SamplerSlot::ShadowMap);
            context->PSSetShaderResources(shadowSRVSlot, 1, m_shadowMapSRV.GetAddressOf());
            context->PSSetSamplers(shadowSamplerSlot, 1, m_shadowSampler.GetAddressOf());
        }

        UpdateSceneBuffer(m_worldSceneData);
        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::World)
                continue;
            ExecuteDrawCommand(cmd);
        }

        //------------------------------------------------------------
        // シャドウマップのバインドを解除（DSVとSRVの同時バインド防止）
        //------------------------------------------------------------
        {
            ID3D11DeviceContext*      context       = m_graphicsContext.GetContext();
            ID3D11ShaderResourceView* nullSRV       = nullptr;
            constexpr UINT            shadowSRVSlot = static_cast<UINT>(SRVSlot::ShadowMap);
            context->PSSetShaderResources(shadowSRVSlot, 1, &nullSRV);
        }

        //------------------------------------------------------------
        // Tonemapパス（HDR → LDR変換してバックバッファへ）
        //------------------------------------------------------------
        ExecuteTonemapPass();

        //------------------------------------------------------------
        // Overlay パス
        //------------------------------------------------------------
        UpdateSceneBuffer(m_overlaySceneData);
        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::Overlay)
                continue;
            ExecuteDrawCommand(cmd);
        }

        m_drawQueue.Clear();

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
        context->PSSetConstantBuffers(0, 1, m_sceneBuffer.GetAddressOf());
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
        // カメラ行列のみ更新し、ライト情報は上書きしない
        m_worldSceneData.view        = data.view;
        m_worldSceneData.projection  = data.projection;
        m_worldSceneData.viewProj    = data.viewProj;
        m_worldSceneData.invViewProj = data.invViewProj;
        m_worldSceneData.cameraPos   = data.cameraPos;    // PBR視線ベクトル用
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
    //! @brief シャドウマップ用リソースの作成
    //------------------------------------------------------------
    bool Renderer::CreateShadowMap() {
        ID3D11Device* device = m_graphicsContext.GetDevice();

        //------------------------------------------------------------
        // シャドウマップテクスチャの作成
        // R32_TYPELESS : DSV(D32_FLOAT)とSRV(R32_FLOAT)で共有するため
        //------------------------------------------------------------
        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width            = SHADOW_MAP_SIZE;
        texDesc.Height           = SHADOW_MAP_SIZE;
        texDesc.MipLevels        = 1;
        texDesc.ArraySize        = 1;
        texDesc.Format           = DXGI_FORMAT_R32_TYPELESS;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage            = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

        HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, m_shadowMapTex.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create shadow map texture.");
            return false;
        }

        //------------------------------------------------------------
        // DSVの作成（深度書き込み用）
        //------------------------------------------------------------
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format        = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

        hr = device->CreateDepthStencilView(m_shadowMapTex.Get(), &dsvDesc, m_shadowMapDSV.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create shadow map DSV.");
            return false;
        }

        //------------------------------------------------------------
        // SRVの作成（PSでのサンプリング用）
        //------------------------------------------------------------
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels       = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        hr = device->CreateShaderResourceView(m_shadowMapTex.Get(), &srvDesc, m_shadowMapSRV.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create shadow map SRV.");
            return false;
        }

        //------------------------------------------------------------
        // PCF用比較サンプラーの作成
        // SampleCmpLevelZero で使用する
        //------------------------------------------------------------
        D3D11_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
        samplerDesc.BorderColor[0] = 1.0f;    // 範囲外は「影なし」にする
        samplerDesc.BorderColor[1] = 1.0f;
        samplerDesc.BorderColor[2] = 1.0f;
        samplerDesc.BorderColor[3] = 1.0f;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_GREATER_EQUAL;
        samplerDesc.MinLOD         = 0;
        samplerDesc.MaxLOD         = D3D11_FLOAT32_MAX;

        hr = device->CreateSamplerState(&samplerDesc, m_shadowSampler.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create shadow sampler.");
            return false;
        }

        return true;
    }

    //------------------------------------------------------------
    //! @brief シャドウパスの実行（シャドウマップへの深度書き込み）
    //------------------------------------------------------------
    void Renderer::ExecuteShadowCommand(const DrawCommand& cmd) {
        if(!cmd.mesh)
            return;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        bool isSkeletal = cmd.boneMatrices && cmd.boneCount > 0;

        //------------------------------------------------------------
        // シャドウ用パイプラインをセット
        //------------------------------------------------------------
        auto* pipeline = isSkeletal ? m_shadowSkeletalPipeline.get() : m_shadowStaticPipeline.get();

        if(!pipeline)
            return;

        m_graphicsContext.SetPipelineState(*pipeline);

        //------------------------------------------------------------
        // Scene (b0) を再バインド
        //------------------------------------------------------------
        context->VSSetConstantBuffers(0, 1, m_sceneBuffer.GetAddressOf());

        //------------------------------------------------------------
        // Transform (b1)
        //------------------------------------------------------------
        CBufferTransform cb{};
        cb.world = cmd.transform;
        context->UpdateSubresource(m_objectBuffer.Get(), 0, nullptr, &cb, 0, 0);
        context->VSSetConstantBuffers(1, 1, m_objectBuffer.GetAddressOf());

        //------------------------------------------------------------
        // ボーン行列 (b3)
        //------------------------------------------------------------
        if(isSkeletal) {
            CBufferSkinning cbSkin{};
            uint32_t        copyCount = std::min(cmd.boneCount, 128u);
            std::memcpy(cbSkin.bones, cmd.boneMatrices, sizeof(hlslpp::float4x4) * copyCount);
            context->UpdateSubresource(m_skinningBuffer.Get(), 0, nullptr, &cbSkin, 0, 0);
            context->VSSetConstantBuffers(3, 1, m_skinningBuffer.GetAddressOf());
        } else {
            ID3D11Buffer* nullBuffer = nullptr;
            context->VSSetConstantBuffers(3, 1, &nullBuffer);
        }

        //------------------------------------------------------------
        // 頂点バッファ・インデックスバッファのセット
        //------------------------------------------------------------
        if(isSkeletal && cmd.mesh->boneWeightBuffer.Get() != nullptr) {
            ID3D11Buffer* vbs[]     = {cmd.mesh->vertexBuffer.Get(), cmd.mesh->boneWeightBuffer.Get()};
            UINT          strides[] = {cmd.mesh->stride, sizeof(Tsukino::GraphicsCommon::BoneWeight)};
            UINT          offsets[] = {0, 0};
            context->IASetVertexBuffers(0, 2, vbs, strides, offsets);
        } else {
            ID3D11Buffer* vbs[]     = {cmd.mesh->vertexBuffer.Get(), nullptr};
            UINT          strides[] = {cmd.mesh->stride, 0};
            UINT          offsets[] = {0, 0};
            context->IASetVertexBuffers(0, 2, vbs, strides, offsets);
        }

        context->IASetIndexBuffer(cmd.mesh->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        //------------------------------------------------------------
        // 描画
        //------------------------------------------------------------
        context->DrawIndexed(cmd.mesh->indexCount, 0, 0);
    }

    //------------------------------------------------------------
    //! @brief シャドウパイプラインのセット
    //------------------------------------------------------------
    void Renderer::SetShadowPipeline(std::shared_ptr<PipelineState> staticPipeline, std::shared_ptr<PipelineState> skeletalPipeline) {
        m_shadowStaticPipeline   = staticPipeline;
        m_shadowSkeletalPipeline = skeletalPipeline;
    }

    //------------------------------------------------------------
    //! @brief 白テクスチャSRVの取得
    //------------------------------------------------------------
    ID3D11ShaderResourceView* Renderer::GetWhiteTextureSRV() {
        return m_whiteSRV.Get();
    }

    //------------------------------------------------------------
    //! @brief 大気散乱パラメータのセット
    //------------------------------------------------------------
    void Renderer::SetSkyParameters(const CBufferSky& sky) {
        m_skyData = sky;
    }

    //------------------------------------------------------------
    //! @brief スカイパイプラインのセット
    //------------------------------------------------------------
    void Renderer::SetSkyPipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps) {
        if(!vs || !ps) {
            Tsukino::Core::Log::Error("Renderer::SetSkyPipeline - shader is null.");
            return;
        }

        ID3D11Device* device = m_graphicsContext.GetDevice();

        HRESULT hr = device->CreateVertexShader(vs->binary.data(), vs->binary.size(), nullptr, m_skyVS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create sky vertex shader.");
            return;
        }

        hr = device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_skyPS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create sky pixel shader.");
            return;
        }

        m_hasSky = true;
    }

    //------------------------------------------------------------
    //! @brief トーンマッピングパイプラインのセット
    //------------------------------------------------------------
    void Renderer::SetTonemapPipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps) {
        if(!vs || !ps) {
            Tsukino::Core::Log::Error("Renderer::SetTonemapPipeline - shader is null.");
            return;
        }

        ID3D11Device* device = m_graphicsContext.GetDevice();

        HRESULT hr = device->CreateVertexShader(vs->binary.data(), vs->binary.size(), nullptr, m_tonemapVS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create tonemap vertex shader.");
            return;
        }

        hr = device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_tonemapPS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create tonemap pixel shader.");
            return;
        }

        m_hasTonemapper = true;
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
            // スロットをクリア
            ID3D11Buffer* nullBuffers[] = {nullptr, nullptr};
            UINT          strides[]     = {0, 0};
            UINT          offsets[]     = {0, 0};
            context->IASetVertexBuffers(0, 2, nullBuffers, strides, offsets);

            // カスタム描画実行
            cmd.customDraw(context);

            // 重要：SpriteBatchで汚されたステートをリセット
            // これを入れないとSpriteの後の描画が真っ暗になったり崩れます
            context->OMSetBlendState(m_commonStatesTK->Opaque(), nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(m_commonStatesTK->DepthDefault(), 0);
            context->RSSetState(m_commonStatesTK->CullNone());

            // s0をLinearWrapに戻す（SpriteBatch汚染対策）
            ID3D11SamplerState* linearWrap = m_samplers[static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::LinearWrap)].Get();
            context->PSSetSamplers(static_cast<UINT>(SamplerSlot::Material), 1, &linearWrap);
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

    //------------------------------------------------------------
    //! @brief ディレクショナルライトの設定
    //------------------------------------------------------------
    void Renderer::SetDirectionalLight(const hlslpp::float3& direction, const hlslpp::float3& color, float intensity) {
        //------------------------------------------------------------
        // ライト方向を正規化
        //------------------------------------------------------------
        hlslpp::float3 normalizedDir = hlslpp::normalize(direction);

        //------------------------------------------------------------
        // lightViewProj の計算
        // ディレクショナルライトは平行投影を使う
        //------------------------------------------------------------

        // ライトの位置はシーンから十分離れた場所に置く
        hlslpp::float3 lightPos = -normalizedDir * 500.0f;
        hlslpp::float3 target   = hlslpp::float3(0.0f, 0.0f, 0.0f);
        hlslpp::float3 up       = hlslpp::float3(0.0f, 1.0f, 0.0f);

        // ライト方向が真上/真下に近いときupベクトルが平行になるので回避
        float dotUp = std::abs(hlslpp::dot(normalizedDir, up));
        if(dotUp > 0.99f) {
            up = hlslpp::float3(0.0f, 0.0f, 1.0f);
        }

        // LookAt でライトのView行列を作成
        Tsukino::Core::Math::matrix lightView = Tsukino::Core::Math::matrix::lookAtLH(lightPos, target, up);
        // 平行投影でライトのProj行列を作成
        Tsukino::Core::Math::matrix lightProj = Tsukino::Core::Math::matrix::orthographicOffCenterLH(-500.0f,    // left
                                                                                                     500.0f,     // right
                                                                                                     -500.0f,    // bottom
                                                                                                     500.0f,     // top
                                                                                                     2000.0f,    // far
                                                                                                     1.0f        // near
        );

        //------------------------------------------------------------
        // m_worldSceneData に書き込む
        //------------------------------------------------------------
        m_worldSceneData.lightViewProj = hlslpp::mul(lightView, lightProj);
        m_worldSceneData.lightDir      = hlslpp::float4(normalizedDir.x, normalizedDir.y, normalizedDir.z, 0.0f);
        m_worldSceneData.lightColor    = hlslpp::float4(color.x, color.y, color.z, intensity);
    }

    //------------------------------------------------------------
    //! @brief 白テクスチャの作成（1x1の白いピクセル）
    //------------------------------------------------------------
    bool Renderer::CreateWhiteTexture() {
        ID3D11Device* device = m_graphicsContext.GetDevice();

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width            = 1;
        desc.Height           = 1;
        desc.MipLevels        = 1;
        desc.ArraySize        = 1;
        desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage            = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

        uint32_t               white = 0xFFFFFFFF;
        D3D11_SUBRESOURCE_DATA initData{&white, 4, 0};

        HRESULT hr = device->CreateTexture2D(&desc, &initData, m_whiteTex.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create white texture.");
            return false;
        }

        hr = device->CreateShaderResourceView(m_whiteTex.Get(), nullptr, m_whiteSRV.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create white texture SRV.");
            return false;
        }

        return true;
    }

    //------------------------------------------------------------
    //! @brief スカイパスの実行
    //------------------------------------------------------------
    void Renderer::ExecuteSkyPass() {
        if(!m_hasSky || !m_skyVS || !m_skyPS)
            return;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        //----------------------------------------------------------
        // シェーダーをセット
        //----------------------------------------------------------
        context->VSSetShader(m_skyVS.Get(), nullptr, 0);
        context->PSSetShader(m_skyPS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);    // 頂点バッファ不要

        //----------------------------------------------------------
        // 深度書き込みなし（スカイは常に最背面）
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_commonStatesTK->DepthRead(), 0);
        context->OMSetBlendState(m_commonStatesTK->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_commonStatesTK->CullNone());

        //----------------------------------------------------------
        // Scene (b0) をバインド（invViewProjの計算に使う）
        //----------------------------------------------------------
        context->VSSetConstantBuffers(0, 1, m_sceneBuffer.GetAddressOf());
        context->PSSetConstantBuffers(0, 1, m_sceneBuffer.GetAddressOf());

        //----------------------------------------------------------
        // Sky (b4) をバインド
        //----------------------------------------------------------
        context->UpdateSubresource(m_skyBuffer.Get(), 0, nullptr, &m_skyData, 0, 0);
        context->PSSetConstantBuffers(4, 1, m_skyBuffer.GetAddressOf());

        //----------------------------------------------------------
        // 頂点バッファなしでフルスクリーントライアングルを描画
        // VSでSV_VertexIDから3頂点を生成する
        //----------------------------------------------------------
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->Draw(3, 0);

        //----------------------------------------------------------
        // ステートをリセット
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_commonStatesTK->DepthDefault(), 0);
    }

    //------------------------------------------------------------
    //! @brief トーンマッピングパスの実行
    //------------------------------------------------------------
    void Renderer::ExecuteTonemapPass() {
        if(!m_hasTonemapper || !m_tonemapVS || !m_tonemapPS)
            return;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        //----------------------------------------------------------
        // バックバッファに切り替え（HDR SRVとRTVの同時バインド防止）
        //----------------------------------------------------------
        m_graphicsContext.BindBackBuffer();

        //----------------------------------------------------------
        // HDRバッファをt0にバインド
        //----------------------------------------------------------
        ID3D11ShaderResourceView* hdrSRV = m_graphicsContext.GetHDRSRV();
        context->PSSetShaderResources(0, 1, &hdrSRV);

        // LinearClampサンプラーをs0にバインド
        ID3D11SamplerState* sampler = m_samplers[static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::LinearClamp)].Get();
        context->PSSetSamplers(0, 1, &sampler);

        //----------------------------------------------------------
        // シェーダーをセット
        //----------------------------------------------------------
        context->VSSetShader(m_tonemapVS.Get(), nullptr, 0);
        context->PSSetShader(m_tonemapPS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        //----------------------------------------------------------
        // 深度なし・ブレンドなし
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_commonStatesTK->DepthNone(), 0);
        context->OMSetBlendState(m_commonStatesTK->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_commonStatesTK->CullNone());

        //----------------------------------------------------------
        // フルスクリーントライアングル描画
        //----------------------------------------------------------
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->Draw(3, 0);

        //----------------------------------------------------------
        // HDR SRVのバインドを解除
        //----------------------------------------------------------
        ID3D11ShaderResourceView* nullSRV = nullptr;
        context->PSSetShaderResources(0, 1, &nullSRV);
    }
}    // namespace Tsukino::Renderer

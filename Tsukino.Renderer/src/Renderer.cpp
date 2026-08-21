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

#include <Tsukino/EngineIntegration/ECS/System/EffectSystem.hpp>

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
    bool Renderer::Initialize(HWND hwnd, uint32_t width, uint32_t height, const RendererShaderSet& shaders) {
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
        if(!CreateShadowPipelines(shaders.shadowStaticVS, shaders.shadowSkeletalVS, shaders.shadowPS)) {
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
        // マテリアル用デフォルトテクスチャ（白・フラット法線）の作成
        //------------------------------------------------------------
        if(!CreateDefaultTextures())
            return false;    // デフォルトテクスチャの作成に失敗した場合は false を返す

        //------------------------------------------------------------
        // デバッグ用バッファの作成
        //------------------------------------------------------------
        if(!CreateDebugBuffers(shaders.debugVS, shaders.debugPS))
            return false;

        //------------------------------------------------------------
        // トーンマッピングパイプラインの作成
        //------------------------------------------------------------
        SetTonemapPipeline(shaders.tonemapVS, shaders.tonemapPS);

        //------------------------------------------------------------
        // シャドウマップ用リソースの作成
        //------------------------------------------------------------
        if(!CreateShadowMap())
            return false;

        //------------------------------------------------------------
        // ディファードLightingパイプラインの作成
        // GBufferパスのPS(gbufferPS)はModelSystem側でPipelineFactory経由の
        // 通常のDrawCommandとして扱うため、ここでは不要。
        //------------------------------------------------------------
        if(!SetLightingPipeline(shaders.lightingPS))
            return false;

        //------------------------------------------------------------
        // モーションブラーパイプラインの作成
        // 演出用の任意機能なので、失敗しても描画自体は続行する
        // （m_hasMotionBlur が false のままになり、パスがスキップされる）。
        //------------------------------------------------------------
        if(!SetMotionBlurPipeline(shaders.motionBlurPS)) {
            Tsukino::Core::Log::Error("Renderer: Motion blur is disabled because its pixel shader could not be created.");
        }

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

        //------------------------------------------------------------
        // m_waterBuffer (b5) の作成
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferWater);
        hr             = device->CreateBuffer(&desc, nullptr, m_waterBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create water constant buffer.");
            return false;
        }

        m_waterData.time         = 0.0f;
        m_waterData.waveSpeed    = -0.08f;
        m_waterData.waveScale    = 1.2f;
        m_waterData.fresnelPower = 4.0f;
        m_waterData.shallowColor = hlslpp::float4(0.2f, 0.6f, 0.5f, 1.0f);
        m_waterData.deepColor    = hlslpp::float4(0.0f, 0.1f, 0.3f, 1.0f);

        //------------------------------------------------------------
        // m_lightsBuffer (b6) の作成（ディファードLightingパス用の点光源・スポットライト配列）
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferLights);
        hr             = device->CreateBuffer(&desc, nullptr, m_lightsBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create lights constant buffer.");
            return false;
        }

        //------------------------------------------------------------
        // m_prevSkinningBuffer (b7) の作成（速度バッファ生成用の前フレームボーン行列）
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferSkinningPrev);
        hr             = device->CreateBuffer(&desc, nullptr, m_prevSkinningBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create previous frame skinning constant buffer.");
            return false;
        }

        //------------------------------------------------------------
        // m_motionBlurBuffer (b8) の作成
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferMotionBlur);
        hr             = device->CreateBuffer(&desc, nullptr, m_motionBlurBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create motion blur constant buffer.");
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
    void Renderer::Render(class Tsukino::BuiltIn::ECS::EffectSystem* effectSystem) {
        m_graphicsContext.BeginFrame(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);

        const auto& commands = m_drawQueue.GetCommands();

        //------------------------------------------------------------
        // 水の更新
        //------------------------------------------------------------
        m_waterData.time = m_waterTime;

        //------------------------------------------------------------
        // Shadow パス（ディファードGBufferの対象＝不透明3Dモデルのみ影を落とす）
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
                if(cmd.pass != RenderPass::GBuffer)
                    continue;
                ExecuteShadowCommand(cmd);
            }

            // RTとビューポートをBeginFrame時の状態に戻す
            m_graphicsContext.BeginFrame(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
        }

        //------------------------------------------------------------
        // Sky パス（GBufferパスの前、深度書き込みなし）
        //------------------------------------------------------------
        UpdateSceneBuffer(m_worldSceneData);
        ExecuteSkyPass();

        //------------------------------------------------------------
        // GBuffer パス（不透明3Dモデル。ライティングは計算せずG-Bufferへ書き込むだけ）
        //------------------------------------------------------------
        UpdateSceneBuffer(m_worldSceneData);
        m_graphicsContext.BeginGBufferPass();
        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::GBuffer)
                continue;
            ExecuteDrawCommand(cmd);
        }

        //------------------------------------------------------------
        // Lighting パス（G-Bufferと深度から全ライトを1回でHDRへ加算する）
        //------------------------------------------------------------
        ExecuteLightingPass();

        //------------------------------------------------------------
        // HDRバッファへ復帰（Lightingの結果を保持したままDSVも再度有効化）
        // 以降のWorld/Transparent/WaterはG-Bufferパスで書いた深度に対して
        // 正しく前後関係が出る（デバッグ線や半透明が不透明オブジェクトの後ろに隠れる）
        //------------------------------------------------------------
        m_graphicsContext.BindHDRRenderTarget();

        //------------------------------------------------------------
        // World パス（デバッグ線などcustomDraw経由のフォワード不透明）
        //------------------------------------------------------------
        {
            ID3D11DeviceContext* context = m_graphicsContext.GetContext();
            // シャドウマップをt8・s8にバインド（フォワードでライティングするシェーダー向け）
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
        // Transparent パス（不透明の後、水面の前）
        //
        // ブレンドと深度の設定は各コマンドの PipelineState が持っているため、
        // ここでは実行順を分けるだけでよい。
        // 半透明同士の前後関係を正しく出すには奥から手前への
        // ソートが必要だが、それはコマンドキュー側の課題として未対応。
        //------------------------------------------------------------
        UpdateSceneBuffer(m_worldSceneData);
        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::Transparent)
                continue;
            ExecuteDrawCommand(cmd);
        }

        UpdateSceneBuffer(m_worldSceneData);
        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::Water)
                continue;
            ExecuteWaterCommand(cmd);
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
        // モーションブラーパス（HDR → ポストプロセス用中間バッファ）
        //
        // 3Dの描画がすべて終わり、UI/エフェクトが乗る前のここで掛ける。
        // 無効なら何もせず false を返すので、その場合はHDRをそのまま
        // トーンマップへ渡す。
        //------------------------------------------------------------
        const bool motionBlurred = ExecuteMotionBlurPass();

        //------------------------------------------------------------
        // Tonemapパス（HDR → LDR変換してバックバッファへ）
        //------------------------------------------------------------
        ExecuteTonemapPass(motionBlurred ? m_graphicsContext.GetPostProcessSRV() : m_graphicsContext.GetHDRSRV());

        //------------------------------------------------------------
        // Overlay パス
        //------------------------------------------------------------
        UpdateSceneBuffer(m_overlaySceneData);
        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::Overlay)
                continue;
            ExecuteDrawCommand(cmd);
        }

        //------------------------------------------------------------
        // エフェクト描画パス
        //------------------------------------------------------------
        if(effectSystem) {
            ID3D11DeviceContext* context = m_graphicsContext.GetContext();
            context->OMSetBlendState(m_commonStatesTK->AlphaBlend(), nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(m_commonStatesTK->DepthNone(), 0);
            effectSystem->RenderEffects(context, m_worldSceneData.view, m_worldSceneData.projection);
            context->OMSetBlendState(m_commonStatesTK->Opaque(), nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(m_commonStatesTK->DepthDefault(), 0);
        }

        m_drawQueue.Clear();

        //------------------------------------------------------------
        // 次フレームの速度計算用に、今フレームのViewProjectionを退避する
        //
        // CameraSystemはdirty時しか行列を再計算しないため、
        // 「送られてきたタイミング」ではなく「フレームの末尾」で
        // 退避するのが確実。
        //------------------------------------------------------------
        m_prevWorldViewProj = m_worldSceneData.viewProj;

        //------------------------------------------------------------
        // モーションブラーの有効フラグはフレーム単位で消す
        //
        // 毎フレームMotionBlurSystemが再度trueにする前提にしておくと、
        // MotionBlurSystemを持たないシーンへ切り替えたときにフラグが
        // 立ちっぱなしで残る問題が起きない（無駄なフルスクリーンパスの防止）。
        //------------------------------------------------------------
        m_motionBlurEnabled = false;

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
            context->OMSetDepthStencilState(m_commonStatesTK->DepthReverseZ(), 0);
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
    //! @brief 描画領域のリサイズ
    //------------------------------------------------------------
    void Renderer::Resize(uint32_t width, uint32_t height) {
        //------------------------------------------------------------
        // GraphicsContext::ClearState() でパイプラインの状態が全て落ちるため、
        // 積み残しの描画コマンドは破棄しておく。
        // コマンドが指す Material / MeshBuffer はシステム側が所有しており、
        // 次フレームの Update で改めて積み直される。
        //------------------------------------------------------------
        m_drawQueue.Clear();
        m_debugLineVertices.clear();
        m_debugTriangleVertices.clear();

        if(!m_graphicsContext.Resize(width, height)) {
            Tsukino::Core::Log::Error("Renderer::Resize - failed to resize the swap chain. Rendering continues at the previous size.");
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
        // 前フレームのViewProjectionを差し込む
        //
        // 呼び出し側（CameraSystem）はこの値を知らないので、
        // Renderer自身が退避しておいたものをここで合流させる。
        //------------------------------------------------------------
        CBufferScene uploadData = sceneData;
        uploadData.prevViewProj = m_prevWorldViewProj;

        //------------------------------------------------------------
        // GPU上のバッファ（m_sceneBuffer）の中身を書き換える
        //------------------------------------------------------------
        context->UpdateSubresource(m_sceneBuffer.Get(), 0, nullptr, &uploadData, 0, 0);

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
    //! @brief 水面描画コマンドの実行
    //------------------------------------------------------------
    void Renderer::ExecuteWaterCommand(const DrawCommand& cmd) {
        if(!cmd.material || !cmd.mesh)
            return;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        //----------------------------------------------------------
        // cmd.material のパイプラインをセット
        //----------------------------------------------------------
        m_graphicsContext.SetMaterial(*cmd.material);

        //----------------------------------------------------------
        // シャドウマップを t8 / s8 にバインド
        // （Water.ps.hlsl は t8/s8 を参照する）
        //----------------------------------------------------------
        context->PSSetShaderResources(8, 1, m_shadowMapSRV.GetAddressOf());
        context->PSSetSamplers(8, 1, m_waterShadowSampler.GetAddressOf());

        //----------------------------------------------------------
        // Scene (b0) を再バインド
        //----------------------------------------------------------
        context->VSSetConstantBuffers(0, 1, m_sceneBuffer.GetAddressOf());
        context->PSSetConstantBuffers(0, 1, m_sceneBuffer.GetAddressOf());

        //----------------------------------------------------------
        // Transform (b1)
        //----------------------------------------------------------
        CBufferTransform cb{};
        cb.world = cmd.transform;
        context->UpdateSubresource(m_objectBuffer.Get(), 0, nullptr, &cb, 0, 0);
        context->VSSetConstantBuffers(1, 1, m_objectBuffer.GetAddressOf());

        //----------------------------------------------------------
        // Water (b5) を更新してバインド
        // time は UpdateWaterTime() で毎フレーム更新済み
        //----------------------------------------------------------
        context->UpdateSubresource(m_waterBuffer.Get(), 0, nullptr, &m_waterData, 0, 0);
        context->PSSetConstantBuffers(5, 1, m_waterBuffer.GetAddressOf());

        //----------------------------------------------------------
        // 頂点バッファ・インデックスバッファのセット
        // 水面はスキニングなし（スロット1は必ずクリア）
        //----------------------------------------------------------
        ID3D11Buffer* vbs[]     = {cmd.mesh->vertexBuffer.Get(), nullptr};
        UINT          strides[] = {cmd.mesh->stride, 0};
        UINT          offsets[] = {0, 0};
        context->IASetVertexBuffers(0, 2, vbs, strides, offsets);
        context->IASetIndexBuffer(cmd.mesh->indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        //----------------------------------------------------------
        // 描画
        //----------------------------------------------------------
        context->DrawIndexed(cmd.mesh->indexCount, 0, 0);

        //----------------------------------------------------------
        // 後片付け：t8/s8 を解除（他のパスへの影響を防ぐ）
        //----------------------------------------------------------
        ID3D11ShaderResourceView* nullSRV = nullptr;
        context->PSSetShaderResources(8, 1, &nullSRV);
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
    //! @brief フラット法線テクスチャのSRVを取得
    //------------------------------------------------------------
    ID3D11ShaderResourceView* Renderer::GetFlatNormalTextureSRV() {
        return m_flatNormalSRV.Get();
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
    //! @brief 水面の時間経過を更新
    //------------------------------------------------------------
    void Renderer::UpdateWaterTime(float deltaTime) {
        m_waterTime      += deltaTime;
        m_waterData.time  = m_waterTime;
    }

    //------------------------------------------------------------
    //! @brief 水面パラメータのセット
    //------------------------------------------------------------
    void Renderer::SetWaterParameters(const CBufferWater& water) {
        m_waterData.waveSpeed    = water.waveSpeed;
        m_waterData.waveScale    = water.waveScale;
        m_waterData.fresnelPower = water.fresnelPower;
        m_waterData.shallowColor = water.shallowColor;
        m_waterData.deepColor    = water.deepColor;
    }

    //------------------------------------------------------------
    //! @brief 水面パイプラインのセット
    //!        PipelineFactory でキャッシュを生成して m_waterPipeline に保持する
    //------------------------------------------------------------
    void Renderer::SetWaterPipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps) {
        if(!vs || !ps) {
            Tsukino::Core::Log::Error("Renderer::SetWaterPipeline - shader is null.");
            return;
        }

        auto* factory = GetPipelineFactory();
        if(!factory)
            return;

        // BlendMode::Alpha で半透明パイプラインをキャッシュ生成
        m_waterPipeline = factory->Create(*vs, *ps, Tsukino::GraphicsCommon::VertexFormat::PositionNormalUV, DepthMode::ReadWrite, BlendMode::Alpha);

        if(!m_waterPipeline) {
            Tsukino::Core::Log::Error("Renderer: Water pipeline creation failed.");
            return;
        }

        //----------------------------------------------------------
        // 水面用 PCF 比較サンプラーを s8 用に作成
        // （既存の m_shadowSampler は s1 にバインドされるため別途用意）
        //----------------------------------------------------------
        if(!m_waterShadowSampler) {
            ID3D11Device*      device = m_graphicsContext.GetDevice();
            D3D11_SAMPLER_DESC samplerDesc{};
            samplerDesc.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
            samplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
            samplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
            samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
            samplerDesc.BorderColor[0] = 1.0f;
            samplerDesc.BorderColor[1] = 1.0f;
            samplerDesc.BorderColor[2] = 1.0f;
            samplerDesc.BorderColor[3] = 1.0f;
            samplerDesc.ComparisonFunc = D3D11_COMPARISON_GREATER_EQUAL;
            samplerDesc.MinLOD         = 0;
            samplerDesc.MaxLOD         = D3D11_FLOAT32_MAX;

            HRESULT hr = device->CreateSamplerState(&samplerDesc, m_waterShadowSampler.GetAddressOf());
            if(FAILED(hr)) {
                Tsukino::Core::Log::Error("Renderer: Failed to create water shadow sampler.");
                return;
            }
        }

        m_hasWater = true;
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
    //! @brief ディファードLightingパイプラインのセット
    //! @note  頂点シェーダーはTonemapと同じフルスクリーン三角形用（m_tonemapVSを共用）のため、
    //!        ここではピクセルシェーダーのみ作成する。
    //------------------------------------------------------------
    bool Renderer::SetLightingPipeline(const Tsukino::Asset::ShaderAsset* ps) {
        if(!ps) {
            Tsukino::Core::Log::Error("Renderer::SetLightingPipeline - shader is null.");
            return false;
        }

        ID3D11Device* device = m_graphicsContext.GetDevice();

        HRESULT hr = device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_lightingPS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create lighting pixel shader.");
            return false;
        }

        m_hasLighting = true;
        return true;
    }

    //------------------------------------------------------------
    //! @brief モーションブラーパイプラインのセット
    //! @note  VSはトーンマッピングと共用（フルスクリーン三角形）なのでPSだけ作る
    //------------------------------------------------------------
    bool Renderer::SetMotionBlurPipeline(const Tsukino::Asset::ShaderAsset* ps) {
        if(!ps) {
            Tsukino::Core::Log::Error("Renderer::SetMotionBlurPipeline - shader is null.");
            return false;
        }

        ID3D11Device* device = m_graphicsContext.GetDevice();

        HRESULT hr = device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_motionBlurPS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create motion blur pixel shader.");
            return false;
        }

        m_hasMotionBlur = true;
        return true;
    }

    //------------------------------------------------------------
    //! @brief モーションブラーパラメータのセット
    //------------------------------------------------------------
    void Renderer::SetMotionBlurParameters(const CBufferMotionBlur& params) {
        m_motionBlurData = params;
    }

    //------------------------------------------------------------
    //! @brief 点光源・スポットライト配列のセット
    //! @note  MAX_LIGHTS を超える分は切り捨て、初回のみ警告を出す
    //------------------------------------------------------------
    void Renderer::SetLights(const GPULight* lights, u32 count) {
        u32 copyCount = std::min(count, MAX_LIGHTS);

        if(count > MAX_LIGHTS && !m_lightOverflowWarned) {
            Tsukino::Core::Log::Error("Renderer::SetLights - light count (" + std::to_string(count) + ") exceeds MAX_LIGHTS ("
                                      + std::to_string(MAX_LIGHTS) + "). Extra lights are dropped.");
            m_lightOverflowWarned = true;
        }

        m_lightsData.lightCount = copyCount;
        if(copyCount > 0) {
            std::memcpy(m_lightsData.lights, lights, sizeof(GPULight) * copyCount);
        }
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

            // SpriteBatchで汚されたステートをリセット
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
        // ------------------------------------------------------------
        // モーションブラーが有効で、かつこのオブジェクトに前フレームの
        // データがあるときだけ速度を出す。それ以外は motionFlags.x = 0 に
        // して、VS側で prevClip = curClip（＝速度ゼロ）へ短絡させる。
        // ------------------------------------------------------------
        const bool writeVelocity = m_motionBlurEnabled && cmd.hasPrevFrame;

        CBufferTransform cb{};
        cb.world       = cmd.transform;
        cb.prevWorld   = writeVelocity ? cmd.prevTransform : cmd.transform;
        cb.motionFlags = hlslpp::float4(writeVelocity ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
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

            // --------------------------------------------------------
            // 前フレームのボーン行列 (b7) の適用
            // スキン1体あたり8KBの転送になるため、速度を出さないときは
            // 転送もバインドも行わない
            // --------------------------------------------------------
            if(writeVelocity && cmd.prevBoneMatrices) {
                CBufferSkinningPrev cbSkinPrev{};
                std::memcpy(cbSkinPrev.bones, cmd.prevBoneMatrices, sizeof(hlslpp::float4x4) * copyCount);

                constexpr UINT prevSkinSlot = static_cast<UINT>(CBSlot::SkinningPrev);
                context->UpdateSubresource(m_prevSkinningBuffer.Get(), 0, nullptr, &cbSkinPrev, 0, 0);
                context->VSSetConstantBuffers(prevSkinSlot, 1, m_prevSkinningBuffer.GetAddressOf());
            }
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
        // どちらの分岐もスロット0と1をまとめて設定するので、
        // ここで先にスロット0だけを設定しても上書きされるだけになる。
        if(cmd.boneMatrices && cmd.boneCount > 0 && cmd.mesh->boneWeightBuffer.Get() != nullptr) {
            // ボーンあり：スロット0と1をバインド
            ID3D11Buffer* vbs[]     = {cmd.mesh->vertexBuffer.Get(), cmd.mesh->boneWeightBuffer.Get()};
            UINT          strides[] = {cmd.mesh->stride, sizeof(Tsukino::GraphicsCommon::BoneWeight)};
            UINT          offsets[] = {0, 0};
            context->IASetVertexBuffers(0, 2, vbs, strides, offsets);
        } else {
            // ボーンなし：スロット0のみバインドし、スロット1は必ず明示的にクリア！
            // クリアしないと直前に描いたスキンメッシュのボーンウェイトが残る
            ID3D11Buffer* vbs[]     = {cmd.mesh->vertexBuffer.Get(), nullptr};
            UINT          strides[] = {cmd.mesh->stride, 0};
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
    //! @brief 1x1のデフォルトテクスチャの作成
    //------------------------------------------------------------
    bool Renderer::Create1x1Texture(u32                                               rgba,
                                    Microsoft::WRL::ComPtr<ID3D11Texture2D>&          outTex,
                                    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV,
                                    const char*                                       debugName) {
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

        D3D11_SUBRESOURCE_DATA initData{&rgba, 4, 0};

        HRESULT hr = device->CreateTexture2D(&desc, &initData, outTex.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error(std::string("Failed to create default texture: ") + debugName + ".");
            return false;
        }

        hr = device->CreateShaderResourceView(outTex.Get(), nullptr, outSRV.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error(std::string("Failed to create default texture SRV: ") + debugName + ".");
            return false;
        }

        return true;
    }

    //------------------------------------------------------------
    //! @brief マテリアル用デフォルトテクスチャの作成
    //! @note  R8G8B8A8_UNORM はメモリ上のバイト順が R,G,B,A なので、
    //!        リトルエンディアンのu32では 0xAABBGGRR の並びになる。
    //------------------------------------------------------------
    bool Renderer::CreateDefaultTextures() {
        // 白 (1,1,1,1)：アルベド/MR/エミッシブ/AOの未設定時。乗算で恒等元になる
        if(!Create1x1Texture(0xFFFFFFFF, m_whiteTex, m_whiteSRV, "white"))
            return false;

        // フラット法線：接空間の(0,0,1) → R=0x80, G=0x80, B=0xFF, A=0xFF
        if(!Create1x1Texture(0xFFFF8080, m_flatNormalTex, m_flatNormalSRV, "flat normal"))
            return false;

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
    //! @brief ディファードLightingパスの実行
    //! @note  G-Bufferと深度をもとに全ライトを1回でHDRバッファへ加算する。
    //!        深度0（Skyパスが描いた背景）はPS側でdiscardして保護する。
    //------------------------------------------------------------
    void Renderer::ExecuteLightingPass() {
        if(!m_hasLighting || !m_tonemapVS || !m_lightingPS)
            return;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        //----------------------------------------------------------
        // HDRバッファのみをRTVにバインド（深度をSRVとして読むためDSVは外す）
        //----------------------------------------------------------
        m_graphicsContext.BindHDRTargetOnly();

        //----------------------------------------------------------
        // シェーダーをセット（VSはTonemapと共用のフルスクリーン三角形用）
        //----------------------------------------------------------
        context->VSSetShader(m_tonemapVS.Get(), nullptr, 0);
        context->PSSetShader(m_lightingPS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        //----------------------------------------------------------
        // 深度テストなし・ブレンドなし（discardで背景ピクセルを保護する）
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_commonStatesTK->DepthNone(), 0);
        context->OMSetBlendState(m_commonStatesTK->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_commonStatesTK->CullNone());

        //----------------------------------------------------------
        // Scene (b0) をバインド
        //----------------------------------------------------------
        context->VSSetConstantBuffers(0, 1, m_sceneBuffer.GetAddressOf());
        context->PSSetConstantBuffers(0, 1, m_sceneBuffer.GetAddressOf());

        //----------------------------------------------------------
        // Lights (b6) を更新してバインド
        //----------------------------------------------------------
        context->UpdateSubresource(m_lightsBuffer.Get(), 0, nullptr, &m_lightsData, 0, 0);
        constexpr UINT lightsCBSlot = static_cast<UINT>(CBSlot::Lights);
        context->PSSetConstantBuffers(lightsCBSlot, 1, m_lightsBuffer.GetAddressOf());

        //----------------------------------------------------------
        // G-Buffer (t9〜t12)、深度 (t13)、ワールド座標 (t14) をバインド
        //----------------------------------------------------------
        ID3D11ShaderResourceView* gbufferSRVs[6] = {
            m_graphicsContext.GetGBufferSRV(0),
            m_graphicsContext.GetGBufferSRV(1),
            m_graphicsContext.GetGBufferSRV(2),
            m_graphicsContext.GetGBufferSRV(3),
            m_graphicsContext.GetDepthSRV(),
            m_graphicsContext.GetGBufferSRV(4),
        };
        constexpr UINT gbufferSRVSlot = static_cast<UINT>(SRVSlot::GBufferAlbedo);
        context->PSSetShaderResources(gbufferSRVSlot, 6, gbufferSRVs);

        //----------------------------------------------------------
        // シャドウマップ (t8/s8) をバインド
        //----------------------------------------------------------
        constexpr UINT shadowSRVSlot     = static_cast<UINT>(SRVSlot::ShadowMap);
        constexpr UINT shadowSamplerSlot = static_cast<UINT>(SamplerSlot::ShadowMap);
        context->PSSetShaderResources(shadowSRVSlot, 1, m_shadowMapSRV.GetAddressOf());
        context->PSSetSamplers(shadowSamplerSlot, 1, m_shadowSampler.GetAddressOf());

        //----------------------------------------------------------
        // G-Bufferサンプラー (s9)：フィルタなしのポイントサンプリング
        //----------------------------------------------------------
        ID3D11SamplerState* pointClamp = m_samplers[static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::PointClamp)].Get();
        constexpr UINT      gbufferSamplerSlot = static_cast<UINT>(SamplerSlot::GBuffer);
        context->PSSetSamplers(gbufferSamplerSlot, 1, &pointClamp);

        //----------------------------------------------------------
        // フルスクリーントライアングル描画
        //----------------------------------------------------------
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->Draw(3, 0);

        //----------------------------------------------------------
        // 後片付け：G-Buffer/深度/シャドウマップのSRVを解除
        // （直後にDSVとして再バインドする深度との同時バインド防止のため必須）
        //----------------------------------------------------------
        ID3D11ShaderResourceView* nullSRVs[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        context->PSSetShaderResources(gbufferSRVSlot, 6, nullSRVs);
        ID3D11ShaderResourceView* nullShadowSRV = nullptr;
        context->PSSetShaderResources(shadowSRVSlot, 1, &nullShadowSRV);

        //----------------------------------------------------------
        // 深度ステートを元に戻す（HDRRenderTarget復帰後のWorld/Transparent/Water用）
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_commonStatesTK->DepthDefault(), 0);
    }

    //------------------------------------------------------------
    //! @brief モーションブラーパスの実行
    //! @note  HDRバッファ(t0)と速度バッファ(t15)を読み、ポストプロセス用
    //!        中間バッファへ書き出す。UIやエフェクトはトーンマップ後に
    //!        バックバッファへ直接描かれるため、ブラーの影響を受けない。
    //------------------------------------------------------------
    bool Renderer::ExecuteMotionBlurPass() {
        if(!m_motionBlurEnabled || !m_hasMotionBlur || !m_tonemapVS || !m_motionBlurPS)
            return false;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        //----------------------------------------------------------
        // ポストプロセス用中間バッファへ切り替え
        // （HDRをSRVとして読むため、HDRをRTVに残したままにはできない）
        //----------------------------------------------------------
        m_graphicsContext.BindPostProcessTarget();

        //----------------------------------------------------------
        // シーンカラー(t0/s0)と速度バッファ(t15/s9)をバインド
        //----------------------------------------------------------
        ID3D11ShaderResourceView* hdrSRV = m_graphicsContext.GetHDRSRV();
        context->PSSetShaderResources(0, 1, &hdrSRV);

        ID3D11SamplerState* linearClamp = m_samplers[static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::LinearClamp)].Get();
        context->PSSetSamplers(0, 1, &linearClamp);

        constexpr UINT            velocitySRVSlot = static_cast<UINT>(SRVSlot::GBufferVelocity);
        ID3D11ShaderResourceView* velocitySRV     = m_graphicsContext.GetGBufferSRV(5);
        context->PSSetShaderResources(velocitySRVSlot, 1, &velocitySRV);

        ID3D11SamplerState* pointClamp = m_samplers[static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::PointClamp)].Get();
        constexpr UINT      gbufferSamplerSlot = static_cast<UINT>(SamplerSlot::GBuffer);
        context->PSSetSamplers(gbufferSamplerSlot, 1, &pointClamp);

        //----------------------------------------------------------
        // パラメータ (b8) を更新してバインド
        //----------------------------------------------------------
        context->UpdateSubresource(m_motionBlurBuffer.Get(), 0, nullptr, &m_motionBlurData, 0, 0);
        constexpr UINT motionBlurCBSlot = static_cast<UINT>(CBSlot::MotionBlur);
        context->PSSetConstantBuffers(motionBlurCBSlot, 1, m_motionBlurBuffer.GetAddressOf());

        //----------------------------------------------------------
        // シェーダーをセット（VSはTonemapと共用のフルスクリーン三角形用）
        //----------------------------------------------------------
        context->VSSetShader(m_tonemapVS.Get(), nullptr, 0);
        context->PSSetShader(m_motionBlurPS.Get(), nullptr, 0);
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
        // 後片付け：次フレームでHDR/G-BufferをRTVとして再バインドするため、
        // SRVのバインドを必ず解除する（ExecuteLightingPassと同じ理由）
        //----------------------------------------------------------
        ID3D11ShaderResourceView* nullSRV = nullptr;
        context->PSSetShaderResources(0, 1, &nullSRV);
        context->PSSetShaderResources(velocitySRVSlot, 1, &nullSRV);

        return true;
    }

    //------------------------------------------------------------
    //! @brief トーンマッピングパスの実行
    //------------------------------------------------------------
    void Renderer::ExecuteTonemapPass(ID3D11ShaderResourceView* source) {
        if(!m_hasTonemapper || !m_tonemapVS || !m_tonemapPS)
            return;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        //----------------------------------------------------------
        // バックバッファに切り替え（入力SRVとRTVの同時バインド防止）
        //----------------------------------------------------------
        m_graphicsContext.BindBackBuffer();

        //----------------------------------------------------------
        // 入力となるシーンカラーをt0にバインド
        // （モーションブラーが走った場合はポストプロセス用中間バッファ、
        //   走らなかった場合はHDRバッファがそのまま渡ってくる）
        //----------------------------------------------------------
        ID3D11ShaderResourceView* hdrSRV = source;
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

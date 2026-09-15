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

#include <Tsukino/Renderer/IPostWorldPass.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/GraphicsCommon/Mesh/MeshPrimitives.hpp>

#include <Tsukino/Core/Log.hpp>

#include <cassert>
#include <d3dcompiler.h>
#include <algorithm>
#include <cstring>

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

        //------------------------------------------------------------
        // 描画で共有する資源（PipelineFactory・共通ステート・サンプラー・
        // プリミティブメッシュ・既定テクスチャ）の作成。
        // 以降のパイプライン生成が PipelineFactory を使うため最初に行う
        //------------------------------------------------------------
        if(!m_resources.Initialize(device, context))
            return false;

        //------------------------------------------------------------
        // シャドウパイプラインの生成
        // ------------------------------------------------------------
        if(!CreateShadowPipelines(shaders.shadowStaticVS, shaders.shadowSkeletalVS, shaders.shadowPS)) {
            Tsukino::Core::Log::Error("Failed to create shadow pipelines.");
            return false;
        }

        //------------------------------------------------------------
        // 定数バッファの作成
        //------------------------------------------------------------
        if(!CreateConstantBuffer())
            return false;    // 定数バッファの作成に失敗した場合は false を返す

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

        if(!SetFogPipeline(shaders.fogPS)) {
            Tsukino::Core::Log::Error("Renderer: Fog is disabled because its pixel shader could not be created.");
        }

        if(!SetAmbientParticlePipeline(shaders.ambientParticleVS, shaders.ambientParticlePS)) {
            Tsukino::Core::Log::Error("Renderer: Ambient particles are disabled because their shaders could not be created.");
        }

        //------------------------------------------------------------
        // IBL（スカイ由来の環境光）用リソースの作成
        // 演出用の任意機能なので、失敗しても描画自体は続行する
        // （m_hasIBLBakeShaders が false のままになり、アンビエントが
        //   常に0扱いになるだけで済む）。
        //------------------------------------------------------------
        if(!CreateIBLResources(shaders)) {
            Tsukino::Core::Log::Error("Renderer: IBL is disabled because its resources could not be created.");
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
        // ボーン行列は1ドローごとに書き換えるうえ1本あたり8KBと大きいため、
        // DEFAULT+UpdateSubresourceではなくDYNAMIC+Map(WRITE_DISCARD)で更新する。
        // 実ボーン数ぶんだけ書けるようになり、転送量とCPU側のゼロ初期化が消える
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferSkinning);
        hr             = device->CreateBuffer(&desc, nullptr, m_skinningBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create skinning constant buffer.");
            return false;
        }

        // descは以降のバッファ作成でも使い回すため、既定（DEFAULT + UpdateSubresource）へ戻す。
        // DYNAMICのままにするとUpdateSubresourceで更新している他のバッファがAPIエラーになる
        desc.Usage          = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;

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
        // b3と同じ理由でDYNAMICにする
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferSkinningPrev);
        hr             = device->CreateBuffer(&desc, nullptr, m_prevSkinningBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create previous frame skinning constant buffer.");
            return false;
        }

        desc.Usage          = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;

        //------------------------------------------------------------
        // m_motionBlurBuffer (b8) の作成
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferMotionBlur);
        hr             = device->CreateBuffer(&desc, nullptr, m_motionBlurBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create motion blur constant buffer.");
            return false;
        }

        //------------------------------------------------------------
        // m_fogBuffer (b9) の作成
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferFog);
        hr             = device->CreateBuffer(&desc, nullptr, m_fogBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create fog constant buffer.");
            return false;
        }

        //------------------------------------------------------------
        // m_ambientParticleBuffer (b9) の作成
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferAmbientParticle);
        hr             = device->CreateBuffer(&desc, nullptr, m_ambientParticleBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create ambient particle constant buffer.");
            return false;
        }

        //------------------------------------------------------------
        // m_iblBuffer (b10) の作成
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferIBL);
        hr             = device->CreateBuffer(&desc, nullptr, m_iblBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create IBL constant buffer.");
            return false;
        }

        //------------------------------------------------------------
        // m_iblBakeBuffer (b11) の作成
        // IBLベイク（キャプチャ/irradiance畳み込み/スペキュラプレフィルタ）実行中だけ使う一時バッファ
        //------------------------------------------------------------
        desc.ByteWidth = sizeof(Tsukino::Renderer::CBufferIBLBake);
        hr             = device->CreateBuffer(&desc, nullptr, m_iblBakeBuffer.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create IBL bake constant buffer.");
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
        auto* factory = m_resources.GetPipelineFactory();
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
    //! @brief 描画処理
    //------------------------------------------------------------
    void Renderer::Render(IPostWorldPass* postWorldPass) {
        m_graphicsContext.BeginFrame(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);

        const auto& commands = m_drawQueue.GetCommands();

        //------------------------------------------------------------
        // 今フレームの描画統計をリセットする（負荷調査用）
        // 実際の加算は各Execute*Commandが DrawIndexed の直前で行うため、
        // 早期returnで描かれなかったコマンドは数に入らない
        //------------------------------------------------------------
        m_frameStats              = FrameStats{};
        m_frameStats.commandCount = static_cast<u32>(commands.size());

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

                // 頂点シェーダーが独自に頂点を組み立てるオブジェクトは、
                // 固定のシャドウ用シェーダーでは形を再現できないので外す
                if(!cmd.castsShadow)
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
        // IBLベイク（スカイのキャプチャ→irradiance畳み込み→スペキュラプレフィルタ）
        //
        // m_hasSkyが立つ（=SkyAtmosphereSystemが初めてスカイパイプラインを
        // 確立した）最初のフレームで一度だけ走る。CombatAndroidには現状
        // day-night系のシステムが無く太陽方向はシーン起動時の1回きりなので、
        // これで十分。将来太陽が動く演出が入ったら、そのシステムが
        // RequestIBLRecapture()を呼べば次フレームでここが再び走る
        // （毎フレーム呼ぶと6+6+36=48回のフルスクリーン三角形描画が
        // 毎フレーム発生するため、呼び出し側でのスロットリングが前提）。
        //
        // ここに置く理由：直前のExecuteSkyPass()でm_skyDataがこのフレームの
        // 太陽方向で確定済みであり、直後のUpdateSceneBuffer(m_worldSceneData)
        // （GBufferパスの直前）が、ここで一時的に書き換えたCBufferScene(b0)を
        // 本来のカメラ値へ確実に戻してくれる
        //------------------------------------------------------------
        if(m_hasSky && m_hasIBLBakeShaders && !m_iblBaked) {
            ExecuteIBLCapturePass();
            ExecuteIBLIrradiancePass();
            ExecuteIBLSpecularPrefilterPass();
            m_iblBaked = true;

            //--------------------------------------------------------
            // ビューポートを画面サイズへ戻す。
            // BeginGBufferPass()はRTV/DSVの張り替えとクリアだけでビューポートには
            // 触れないため、ここで戻しておかないと直後のGBufferパスが
            // IBLベイク最後の面（32px/4px等）のままの極小ビューポートで描かれてしまう
            //--------------------------------------------------------
            D3D11_VIEWPORT vp{};
            vp.Width    = static_cast<float>(m_graphicsContext.GetWidth());
            vp.Height   = static_cast<float>(m_graphicsContext.GetHeight());
            vp.MaxDepth = 1.0f;
            m_graphicsContext.GetContext()->RSSetViewports(1, &vp);
        }

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
        // 以降のWorld/TransparentはG-Bufferパスで書いた深度に対して
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

            // IBL (b10, t17〜t19, s10)：Model.ps.hlslのEvaluateIBL向け。
            // World/TransparentDepth/Transparentの3パスを通して張りっぱなしにする
            BindIBLResources();
        }

        UpdateSceneBuffer(m_worldSceneData);
        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::World)
                continue;
            ExecuteDrawCommand(cmd);
        }

        //------------------------------------------------------------
        // TransparentDepth パス（Transparentの直前。色を書かず深度だけ埋める）
        //
        // 半透明モデル（スキンメッシュの自己重なりが多い）が同じピクセルへ
        // 何度もブレンドされて過度に濃く見える問題を避けるため、先に一番手前の
        // 深度だけを確定させておく。Transparent側はEqualReadOnlyでこの深度と
        // 一致する画素だけを1回シェーディングする
        //------------------------------------------------------------
        UpdateSceneBuffer(m_worldSceneData);
        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::TransparentDepth)
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

        //------------------------------------------------------------
        // シャドウマップ・IBLのバインドを解除（DSVとSRVの同時バインド防止）
        //------------------------------------------------------------
        {
            ID3D11DeviceContext*      context       = m_graphicsContext.GetContext();
            ID3D11ShaderResourceView* nullSRV       = nullptr;
            constexpr UINT            shadowSRVSlot = static_cast<UINT>(SRVSlot::ShadowMap);
            context->PSSetShaderResources(shadowSRVSlot, 1, &nullSRV);
            UnbindIBLResources();
        }

        //------------------------------------------------------------
        // 環境パーティクルパス（HDRバッファへ加算合成）
        //
        // 深度テストありなので世界の物体にきちんと隠れ、深度は書かないので
        // 粒子どうしは順序を気にせず重なる。フォグの前に置くことで、
        // 「3Dの絵すべてにフォグが乗る」という下のパスの前提を壊さずに済む
        // （粒子だけがフォグの掛かっていない浮いた点に見えるのを防ぐ）。
        //------------------------------------------------------------
        ExecuteAmbientParticlePass();

        //------------------------------------------------------------
        // フォグパス（HDRバッファへ直接over合成）
        //
        // 不透明・空・半透明・水面がすべて描き終わったここで掛けることで、
        // 3Dの絵すべてにフォグが乗る。UIとエフェクトはトーンマップ後に
        // バックバッファへ直接描かれるため影響を受けない。
        //------------------------------------------------------------
        ExecuteFogPass();

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
        // エフェクト描画パス（Overlayより前）
        //
        // エフェクトは3D空間の演出なので、UIより奥に描く。Overlayの後に置くと
        // 画面を覆うモーダル（スキル選択の暗転板など）の上にヒットエフェクトだけが
        // 残ってしまう
        //------------------------------------------------------------
        if(postWorldPass) {
            ID3D11DeviceContext* context = m_graphicsContext.GetContext();
            context->OMSetBlendState(m_resources.GetCommonStatesTK()->AlphaBlend(), nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthNone(), 0);
            postWorldPass->RenderPostWorld(context, m_worldSceneData.view, m_worldSceneData.projection);
            context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthDefault(), 0);
        }

        //------------------------------------------------------------
        // Overlay パス
        //
        // UIはスプライトも文字も同じRenderPass::Overlayへ積まれる。ここで
        // DrawCommand::sortOrderの昇順へ並べ直さないと「積んだ順＝Systemの登録順」が
        // 優先され、スプライトと文字の前後をsortOrderで指定できない
        // （ダメージ数値がスキル選択カードの上に出る類の不具合になる）。
        //
        // 同じsortOrder同士は積んだ順を保たなければならない。保たないと
        // EnTTのプール順の揺れがそのまま描画順のちらつきとして出る。
        // 比較関数に添字を混ぜているのは、そうすればキーが同値のとき添字の
        // 昇順＝積んだ順に落ちるためで、一時バッファを取るstd::stable_sortを
        // 使わずに安定ソートと同じ結果が得られる
        //------------------------------------------------------------
        UpdateSceneBuffer(m_overlaySceneData);

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

        for(u32 index : m_overlayOrder) {
            ExecuteDrawCommand(commands[index]);
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

        //------------------------------------------------------------
        // フォグの有効フラグもフレーム単位で消す（理由はモーションブラーと同じ）
        //------------------------------------------------------------
        m_fogEnabled = false;

        //------------------------------------------------------------
        // 環境パーティクルの有効フラグもフレーム単位で消す（理由は上と同じ）
        //------------------------------------------------------------
        m_ambientParticleEnabled = false;

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
            context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthReverseZ(), 0);
            // レンダリングステート設定（ここでは必要に応じてワイヤーフレーム用等の設定が必要になる可能性）
            context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

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
                    context->RSSetState(m_resources.GetCommonStatesTK()->Wireframe());    // 三角形はワイヤーフレームで描画
                    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    context->Draw((UINT)count, 0);
                    context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());    // 元に戻す
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
        // コマンドが指す Material / CBufferMaterial はキューが所有しているので
        // Clear() で一緒に破棄される。次フレームの Update で改めて積み直される。
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
    //! @brief このフレームで使うマテリアル実体を1つ確保する
    //------------------------------------------------------------
    Material& Renderer::AllocMaterial() {
        return m_drawQueue.AllocMaterial();
    }

    //------------------------------------------------------------
    //! @brief このフレームで使うマテリアル定数データを1つ確保する
    //------------------------------------------------------------
    CBufferMaterial& Renderer::AllocMaterialData() {
        return m_drawQueue.AllocMaterialData();
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
        // フレーム共通の素材（時間・解像度・シャドウマップ寸法）を差し込む。
        //
        // prevViewProjと同じ理由でここに集約している：呼び出し側
        // （CameraSystem）はこれらを知らないし、知る必要も無い。
        // ここで埋めておけば、どのシェーダーもb0を宣言するだけで
        // 時間や画面サイズを使えるようになり、演出ごとに専用の
        // 定数バッファを1本ずつ確保する必要が無くなる
        //------------------------------------------------------------
        uploadData.timeParams = hlslpp::float4(m_elapsedTime, m_frameDeltaTime, std::sin(m_elapsedTime), std::cos(m_elapsedTime));

        const float screenWidth  = static_cast<float>(m_graphicsContext.GetWidth());
        const float screenHeight = static_cast<float>(m_graphicsContext.GetHeight());
        uploadData.screenParams  = hlslpp::float4(screenWidth, screenHeight, screenWidth > 0.0f ? 1.0f / screenWidth : 0.0f,
                                                  screenHeight > 0.0f ? 1.0f / screenHeight : 0.0f);

        constexpr float shadowMapSize = static_cast<float>(SHADOW_MAP_SIZE);
        uploadData.shadowParams       = hlslpp::float4(shadowMapSize, 1.0f / shadowMapSize, 0.0f, 0.0f);

        //------------------------------------------------------------
        // GPU上のバッファ（m_sceneBuffer）の中身を書き換える
        //------------------------------------------------------------
        context->UpdateSubresource(m_sceneBuffer.Get(), 0, nullptr, &uploadData, 0, 0);

        //------------------------------------------------------------
        // スロット0（b0）にバインドする
        //------------------------------------------------------------
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_sceneBuffer.GetAddressOf());
        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_sceneBuffer.GetAddressOf());
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
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_sceneBuffer.GetAddressOf());

        //------------------------------------------------------------
        // Transform (b1)
        //------------------------------------------------------------
        CBufferTransform cb{};
        cb.world = cmd.transform;
        context->UpdateSubresource(m_objectBuffer.Get(), 0, nullptr, &cb, 0, 0);
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Transform), 1, m_objectBuffer.GetAddressOf());

        //------------------------------------------------------------
        // ボーン行列 (b3)
        //------------------------------------------------------------
        if(isSkeletal) {
            m_shadowBoneBytes = UploadBoneMatrices(m_skinningBuffer.Get(), cmd.boneMatrices, cmd.boneCount);
            context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Skinning), 1, m_skinningBuffer.GetAddressOf());
        } else {
            m_shadowBoneBytes = 0;
            ID3D11Buffer* nullBuffer = nullptr;
            context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Skinning), 1, &nullBuffer);
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
        // インスタンスごとのデータのバインド。
        // これが無いとインスタンス描画したオブジェクトが影を落とさない
        //------------------------------------------------------------
        BindInstanceData(cmd.instanceData);

        // ゲームが自前のシェーダーで描くときのパラメータ。使わないコマンドでも
        // 明示的に空を書き、直前のコマンドのバッファを引き継がせない
        BindUserConstantBuffer(cmd.userConstantBuffer, cmd.userConstantSlot);

        //------------------------------------------------------------
        // 描画
        //------------------------------------------------------------
        if(cmd.instanceCount > 1) {
            context->DrawIndexedInstanced(cmd.mesh->indexCount, cmd.instanceCount, 0, 0, 0);
        } else {
            context->DrawIndexed(cmd.mesh->indexCount, 0, 0);
        }

        //------------------------------------------------------------
        // 統計の加算（負荷調査用）
        // GBufferと同じ形状をもう一度描いているので、ドロー数もボーン転送量も
        // ここで二重に計上されるのが実態どおり
        //------------------------------------------------------------
        ++m_frameStats.shadowDrawCalls;
        m_frameStats.triangleCount += (cmd.mesh->indexCount * cmd.instanceCount) / 3;
        if(isSkeletal) {
            ++m_frameStats.skinnedDrawCalls;
            m_frameStats.boneBytesUploaded += m_shadowBoneBytes;
        }
    }

    //------------------------------------------------------------
    //! @brief ボーン行列を定数バッファへ転送する
    //------------------------------------------------------------
    u32 Renderer::UploadBoneMatrices(ID3D11Buffer* buffer, const void* boneMatrices, u32 boneCount) {
        if(!buffer || !boneMatrices || boneCount == 0)
            return 0;

        // シェーダー側の宣言（float4x4 bones[128]）を超えて書かない
        const u32 copyCount = (boneCount < kMaxBoneCount) ? boneCount : kMaxBoneCount;
        const u32 byteCount = copyCount * static_cast<u32>(sizeof(hlslpp::float4x4));

        //--------------------------------------------------------
        // WRITE_DISCARDで新しい領域をもらい、実ボーン数ぶんだけ書く。
        //
        // 以前は8KBの構造体をスタックにゼロ初期化で作り、実ボーン数ぶんmemcpyしてから
        // UpdateSubresourceで8KB全体を転送していた。Mixamoのリグは65本程度なので、
        // ゼロ初期化と転送の半分以上が捨てられていたことになる。
        // スキンメッシュはShadowとGBufferの2パスで描かれるため、この無駄は2倍で効く
        //--------------------------------------------------------
        ID3D11DeviceContext*     context = m_graphicsContext.GetContext();
        D3D11_MAPPED_SUBRESOURCE mapped{};

        if(FAILED(context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            return 0;

        std::memcpy(mapped.pData, boneMatrices, byteCount);
        context->Unmap(buffer, 0);

        return byteCount;
    }

    //------------------------------------------------------------
    //! @brief パス別のドローコール数と三角形数を数える
    //------------------------------------------------------------
    void Renderer::CountDrawCall(RenderPass pass, u32 indexCount) {
        switch(pass) {
        case RenderPass::GBuffer:          ++m_frameStats.gbufferDrawCalls; break;
        case RenderPass::World:            ++m_frameStats.worldDrawCalls; break;
        case RenderPass::TransparentDepth: ++m_frameStats.transparentDrawCalls; break;
        case RenderPass::Transparent:      ++m_frameStats.transparentDrawCalls; break;
        case RenderPass::Overlay:          ++m_frameStats.overlayDrawCalls; break;
        }

        m_frameStats.triangleCount += indexCount / 3;
    }

    //------------------------------------------------------------
    //! インスタンスごとのデータを頂点シェーダーへバインドします。
    //------------------------------------------------------------
    void Renderer::BindInstanceData(ID3D11ShaderResourceView* srv) {
        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        constexpr UINT slot = static_cast<UINT>(SRVSlot::InstanceData);

        // nullptrのときも「空を書き込む」のが肝。ここを素通りさせると、
        // 直前のインスタンス描画が残したSRVを次のコマンドが読んでしまう
        ID3D11ShaderResourceView* views[] = {srv};
        context->VSSetShaderResources(slot, 1, views);
    }

    //------------------------------------------------------------
    //! ゲーム定義の定数バッファをバインドします。
    //------------------------------------------------------------
    void Renderer::BindUserConstantBuffer(ID3D11Buffer* buffer, CBSlot slot) {
        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        //--------------------------------------------------------------
        // ゲーム予約枠（User0 / User1）以外を指定されたら何もしない。
        // エンジンが使う b0〜b9 を上書きされると描画が壊れるため、
        // ここで弾いておく
        //--------------------------------------------------------------
        if(slot != CBSlot::User0 && slot != CBSlot::User1)
            return;

        // BindInstanceDataと同じ理由で、nullptrのときも明示的に空を書く
        ID3D11Buffer* buffers[] = {buffer};

        const UINT slotIndex = static_cast<UINT>(slot);
        context->VSSetConstantBuffers(slotIndex, 1, buffers);
        context->PSSetConstantBuffers(slotIndex, 1, buffers);
    }

    //------------------------------------------------------------
    //! @brief シャドウパイプラインのセット
    //------------------------------------------------------------
    void Renderer::SetShadowPipeline(std::shared_ptr<PipelineState> staticPipeline, std::shared_ptr<PipelineState> skeletalPipeline) {
        m_shadowStaticPipeline   = staticPipeline;
        m_shadowSkeletalPipeline = skeletalPipeline;
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
    //! @brief フォグパイプラインのセット
    //------------------------------------------------------------
    bool Renderer::SetFogPipeline(const Tsukino::Asset::ShaderAsset* ps) {
        if(!ps) {
            Tsukino::Core::Log::Error("Renderer::SetFogPipeline - shader is null.");
            return false;
        }

        ID3D11Device* device = m_graphicsContext.GetDevice();

        HRESULT hr = device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_fogPS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create fog pixel shader.");
            return false;
        }

        m_hasFog = true;
        return true;
    }

    //------------------------------------------------------------
    //! @brief フォグパラメータのセット
    //------------------------------------------------------------
    void Renderer::SetFogParameters(const CBufferFog& params) {
        m_fogData = params;
    }

    //------------------------------------------------------------
    //! 環境パーティクルのパイプラインをセットします。
    //------------------------------------------------------------
    bool Renderer::SetAmbientParticlePipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps) {
        if(!vs || !ps) {
            Tsukino::Core::Log::Error("Renderer::SetAmbientParticlePipeline - shader is null.");
            return false;
        }

        ID3D11Device* device = m_graphicsContext.GetDevice();

        HRESULT hr = device->CreateVertexShader(vs->binary.data(), vs->binary.size(), nullptr, m_ambientParticleVS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create ambient particle vertex shader.");
            return false;
        }

        hr = device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_ambientParticlePS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to create ambient particle pixel shader.");
            return false;
        }

        m_hasAmbientParticle = true;
        return true;
    }

    //------------------------------------------------------------
    //! 環境パーティクルのパラメータをセットします。
    //------------------------------------------------------------
    void Renderer::SetAmbientParticleParameters(const CBufferAmbientParticle& params, u32 particleCount) {
        m_ambientParticleData  = params;
        m_ambientParticleCount = (particleCount < kMaxAmbientParticles) ? particleCount : kMaxAmbientParticles;
    }

    //------------------------------------------------------------
    //! フレームの経過時間を進めます。
    //------------------------------------------------------------
    void Renderer::AdvanceFrameTime(float deltaTime) {
        m_frameDeltaTime = deltaTime;
        m_elapsedTime += deltaTime;

        //--------------------------------------------------------------
        // floatの精度が落ちて時間の刻みが粗くなるのを防ぐため、
        // 一定時間で折り返す。sin/cosを使う演出が大半なので、
        // 2πの整数倍で折り返せば見た目に不連続は出ない
        //--------------------------------------------------------------
        constexpr float kTimeWrap = 6.28318531f * 1000.0f;    // 約6283秒（1時間45分）
        if(m_elapsedTime > kTimeWrap)
            m_elapsedTime -= kTimeWrap;
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
            context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
            context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthDefault(), 0);
            context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

            // s0をLinearWrapに戻す（SpriteBatch汚染対策）
            ID3D11SamplerState* linearWrap = m_resources.GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearWrap);
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
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_sceneBuffer.GetAddressOf());

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
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Transform), 1, m_objectBuffer.GetAddressOf());

        // ------------------------------------------------------------
        // ボーン行列 (b3) の適用
        // ------------------------------------------------------------
        if(cmd.boneMatrices && cmd.boneCount > 0) {
            m_lastDrawBoneBytes = UploadBoneMatrices(m_skinningBuffer.Get(), cmd.boneMatrices, cmd.boneCount);
            context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Skinning), 1, m_skinningBuffer.GetAddressOf());

            // --------------------------------------------------------
            // 前フレームのボーン行列 (b7) の適用
            // スキン1体あたり8KBの転送になるため、速度を出さないときは
            // 転送もバインドも行わない
            // --------------------------------------------------------
            if(writeVelocity && cmd.prevBoneMatrices) {
                constexpr UINT prevSkinSlot = static_cast<UINT>(CBSlot::SkinningPrev);
                m_lastDrawBoneBytes += UploadBoneMatrices(m_prevSkinningBuffer.Get(), cmd.prevBoneMatrices, cmd.boneCount);
                context->VSSetConstantBuffers(prevSkinSlot, 1, m_prevSkinningBuffer.GetAddressOf());
            }
        } else {
            m_lastDrawBoneBytes = 0;
            // スキニングを使わないオブジェクトを描画するときは、
            // スロット3を nullptr でクリアして、前のオブジェクトのボーン行列が残らないようにする
            ID3D11Buffer* nullBuffer = nullptr;
            context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Skinning), 1, &nullBuffer);
        }

        //------------------------------------------------------
        // Material を適用
        //------------------------------------------------------
        m_graphicsContext.SetMaterial(*cmd.material);

        if(cmd.materialData) {
            context->UpdateSubresource(m_materialBuffer.Get(), 0, nullptr, cmd.materialData, 0, 0);
            context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Material), 1, m_materialBuffer.GetAddressOf());
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
        // インスタンスごとのデータを頂点シェーダーへバインドする。
        // 使わないコマンドが直前のSRVを引き継がないよう、無いときは明示的に外す
        //------------------------------------------------------
        BindInstanceData(cmd.instanceData);

        // ゲームが自前のシェーダーで描くときのパラメータ。使わないコマンドでも
        // 明示的に空を書き、直前のコマンドのバッファを引き継がせない
        BindUserConstantBuffer(cmd.userConstantBuffer, cmd.userConstantSlot);

        //------------------------------------------------------
        // 描画
        //------------------------------------------------------
        if(cmd.instanceCount > 1) {
            context->DrawIndexedInstanced(cmd.mesh->indexCount, cmd.instanceCount, 0, 0, 0);
        } else {
            // 既定値が1なので、既存の描画はすべてこちらを通り続ける
            context->DrawIndexed(cmd.mesh->indexCount, 0, 0);
        }

        //------------------------------------------------------
        // 統計の加算（負荷調査用）
        //------------------------------------------------------
        CountDrawCall(cmd.pass, cmd.mesh->indexCount * cmd.instanceCount);
        if(cmd.boneMatrices && cmd.boneCount > 0) {
            ++m_frameStats.skinnedDrawCalls;
            m_frameStats.boneBytesUploaded += m_lastDrawBoneBytes;
        }
    }

    //------------------------------------------------------------
    //! @brief ディレクショナルライトの設定
    //------------------------------------------------------------
    void Renderer::SetDirectionalLight(const hlslpp::float3& direction, const hlslpp::float3& color, float intensity, const hlslpp::float3& focusPoint) {
        //------------------------------------------------------------
        // ライト方向を正規化
        //------------------------------------------------------------
        hlslpp::float3 normalizedDir = hlslpp::normalize(direction);

        //------------------------------------------------------------
        // lightViewProj の計算
        // ディレクショナルライトは平行投影を使う
        //------------------------------------------------------------

        // シャドウの投影範囲は呼び出し側から渡されたfocusPoint（通常はメインカメラの
        // 注視点）を中心にする（ワールド原点固定だと、カメラが原点から離れる
        // プレイヤー追従型のシーンで影が一切映らなくなる）。
        // カメラ位置そのものを中心にしないのは、TPSカメラのように注視点から
        // 離れた位置にカメラを置く構成だと、画面に映る注視点付近がシャドウ範囲の
        // 端に寄ってしまい、キャラクターのすぐ近くで影が途切れて見えるため
        hlslpp::float3 target   = focusPoint;
        // ライトの位置はターゲットから十分離れた場所に置く
        hlslpp::float3 lightPos = target - normalizedDir * 500.0f;
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
        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthRead(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

        //----------------------------------------------------------
        // Scene (b0) をバインド（invViewProjの計算に使う）
        //----------------------------------------------------------
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_sceneBuffer.GetAddressOf());
        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_sceneBuffer.GetAddressOf());

        //----------------------------------------------------------
        // Sky (b4) をバインド
        //----------------------------------------------------------
        context->UpdateSubresource(m_skyBuffer.Get(), 0, nullptr, &m_skyData, 0, 0);
        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Sky), 1, m_skyBuffer.GetAddressOf());

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
        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthDefault(), 0);
    }

    //------------------------------------------------------------
    //! @brief IBL用リソース（キューブマップ・BRDF LUT・ベイク用PS）の作成
    //------------------------------------------------------------
    bool Renderer::CreateIBLResources(const RendererShaderSet& shaders) {
        ID3D11Device* device = m_graphicsContext.GetDevice();

        constexpr DXGI_FORMAT kIBLCubeFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;    // HDR：sunIntensityは1.0を大きく超える

        //------------------------------------------------------------
        // キューブマップ本体3枚
        //------------------------------------------------------------
        m_iblCaptureCube             = std::make_unique<DX11TextureCube>(kIBLCaptureSize, 1, kIBLCubeFormat, device);
        m_iblIrradianceCube          = std::make_unique<DX11TextureCube>(kIBLIrradianceSize, 1, kIBLCubeFormat, device);
        m_iblPrefilteredSpecularCube = std::make_unique<DX11TextureCube>(kIBLSpecularBaseSize, kIBLSpecularMipCount, kIBLCubeFormat, device);

        if(!m_iblCaptureCube->IsValid() || !m_iblIrradianceCube->IsValid() || !m_iblPrefilteredSpecularCube->IsValid()) {
            Tsukino::Core::Log::Error("Renderer: Failed to create one or more IBL cube maps.");
            return false;
        }

        //------------------------------------------------------------
        // BRDF LUT（スカイに依存しない2D、r: スケールA, g: バイアスB）
        //------------------------------------------------------------
        D3D11_TEXTURE2D_DESC lutDesc = {};
        lutDesc.Width                = kIBLBRDFLUTSize;
        lutDesc.Height               = kIBLBRDFLUTSize;
        lutDesc.MipLevels            = 1;
        lutDesc.ArraySize            = 1;
        lutDesc.Format               = DXGI_FORMAT_R16G16_FLOAT;
        lutDesc.SampleDesc.Count     = 1;
        lutDesc.Usage                = D3D11_USAGE_DEFAULT;
        lutDesc.BindFlags            = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        HRESULT hr = device->CreateTexture2D(&lutDesc, nullptr, m_iblBRDFLUTTex.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Renderer: Failed to create IBL BRDF LUT texture.");
            return false;
        }

        hr = device->CreateRenderTargetView(m_iblBRDFLUTTex.Get(), nullptr, m_iblBRDFLUTRTV.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Renderer: Failed to create IBL BRDF LUT RTV.");
            return false;
        }

        hr = device->CreateShaderResourceView(m_iblBRDFLUTTex.Get(), nullptr, m_iblBRDFLUTSRV.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Renderer: Failed to create IBL BRDF LUT SRV.");
            return false;
        }

        //------------------------------------------------------------
        // ベイク用PS（VSはすべてm_tonemapVS/m_skyVSを共用するのでここでは作らない）
        //------------------------------------------------------------
        if(!shaders.iblIrradiancePS || !shaders.iblSpecularPrefilterPS || !shaders.iblBRDFLUTPS) {
            Tsukino::Core::Log::Error("Renderer::CreateIBLResources - shader is null.");
            return false;
        }

        hr = device->CreatePixelShader(shaders.iblIrradiancePS->binary.data(), shaders.iblIrradiancePS->binary.size(), nullptr,
                                       m_iblIrradiancePS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Renderer: Failed to create IBL irradiance pixel shader.");
            return false;
        }

        hr = device->CreatePixelShader(shaders.iblSpecularPrefilterPS->binary.data(), shaders.iblSpecularPrefilterPS->binary.size(),
                                       nullptr, m_iblSpecularPrefilterPS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Renderer: Failed to create IBL specular prefilter pixel shader.");
            return false;
        }

        hr = device->CreatePixelShader(shaders.iblBRDFLUTPS->binary.data(), shaders.iblBRDFLUTPS->binary.size(), nullptr,
                                       m_iblBRDFLUTPS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Renderer: Failed to create IBL BRDF LUT pixel shader.");
            return false;
        }

        m_hasIBLBakeShaders = true;

        //------------------------------------------------------------
        // 定数バッファの初期値（プレフィルタ済みスペキュラのミップ数はここで確定する）
        //------------------------------------------------------------
        m_iblData.specularMipCount = static_cast<float>(m_iblPrefilteredSpecularCube->GetMipLevels());
        m_iblData.iblIntensity     = 1.0f;
        m_graphicsContext.GetContext()->UpdateSubresource(m_iblBuffer.Get(), 0, nullptr, &m_iblData, 0, 0);

        //------------------------------------------------------------
        // BRDF LUTはスカイに依存しないため、ここで即座に一度だけベイクする
        // （キャプチャ/irradiance/プレフィルタはスカイパイプライン確立後にRender()側でトリガーされる）
        //------------------------------------------------------------
        ExecuteIBLBRDFLUTPass();

        return true;
    }

    //------------------------------------------------------------
    //! @brief 原点から見たキューブの1面ぶんのCBufferSceneを組み立てる
    //! @note  D3D11のキューブ面の並び（+X,-X,+Y,-Y,+Z,-Z）に合わせてある
    //------------------------------------------------------------
    CBufferScene Renderer::BuildCubeFaceSceneData(u32 face) const {
        static const hlslpp::float3 kFaceTargets[6] = {
            hlslpp::float3(1.0f, 0.0f, 0.0f),
            hlslpp::float3(-1.0f, 0.0f, 0.0f),
            hlslpp::float3(0.0f, 1.0f, 0.0f),
            hlslpp::float3(0.0f, -1.0f, 0.0f),
            hlslpp::float3(0.0f, 0.0f, 1.0f),
            hlslpp::float3(0.0f, 0.0f, -1.0f),
        };
        static const hlslpp::float3 kFaceUps[6] = {
            hlslpp::float3(0.0f, 1.0f, 0.0f),
            hlslpp::float3(0.0f, 1.0f, 0.0f),
            hlslpp::float3(0.0f, 0.0f, -1.0f),
            hlslpp::float3(0.0f, 0.0f, 1.0f),
            hlslpp::float3(0.0f, 1.0f, 0.0f),
            hlslpp::float3(0.0f, 1.0f, 0.0f),
        };

        const u32            idx = (face < 6) ? face : 0;
        const hlslpp::float3 eye = hlslpp::float3(0.0f, 0.0f, 0.0f);

        // lightDir/lightColor/timeParams等はメインカメラのフレームデータをそのまま引き継ぐ
        // （Sky.ps.hlslが太陽の色・向きとしてlightColor/lightDirを読むため）
        CBufferScene data = m_worldSceneData;

        constexpr float kFovRadians = 1.5707963267948966f;    // 90度：キューブの1面をちょうど覆う画角
        constexpr float kNearZ      = 0.1f;
        constexpr float kFarZ       = 100.0f;

        data.view        = Tsukino::Core::Math::matrix::lookAtLH(eye, eye + kFaceTargets[idx], kFaceUps[idx]);
        // リバースZ対応のperspectiveFovLHはCameraSystemと同じくfarZ, nearZの順で渡す
        data.projection  = Tsukino::Core::Math::matrix::perspectiveFovLH(kFovRadians, 1.0f, kFarZ, kNearZ);
        data.viewProj    = hlslpp::mul(data.view, data.projection);
        data.invViewProj = hlslpp::inverse(data.viewProj);
        data.cameraPos   = hlslpp::float4(eye.x, eye.y, eye.z, 1.0f);

        return data;
    }

    //------------------------------------------------------------
    //! @brief IBLキャプチャパスの実行（スカイを6面のキューブマップへ焼く）
    //------------------------------------------------------------
    void Renderer::ExecuteIBLCapturePass() {
        if(!m_iblCaptureCube || !m_iblCaptureCube->IsValid() || !m_skyVS || !m_skyPS)
            return;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        context->VSSetShader(m_skyVS.Get(), nullptr, 0);
        context->PSSetShader(m_skyPS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        // 深度バッファを持たないオフスクリーンキャプチャなので深度テストなし
        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

        D3D11_VIEWPORT vp{};
        vp.Width    = static_cast<float>(kIBLCaptureSize);
        vp.Height   = static_cast<float>(kIBLCaptureSize);
        vp.MaxDepth = 1.0f;
        context->RSSetViewports(1, &vp);

        // Sky (b4) は直前のExecuteSkyPass()で今フレーム分がGPUへ転送済みなのでそのままバインドする
        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Sky), 1, m_skyBuffer.GetAddressOf());

        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for(u32 face = 0; face < 6; ++face) {
            CBufferScene faceScene = BuildCubeFaceSceneData(face);
            context->UpdateSubresource(m_sceneBuffer.Get(), 0, nullptr, &faceScene, 0, 0);
            context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_sceneBuffer.GetAddressOf());

            ID3D11RenderTargetView* rtv = m_iblCaptureCube->GetFaceRTV(face, 0);
            context->OMSetRenderTargets(1, &rtv, nullptr);

            context->Draw(3, 0);
        }

        //----------------------------------------------------------
        // RTVバインドを解除する。直後のirradiance畳み込み/スペキュラプレフィルタが
        // この同じキューブをSRV（t20）として読むため、RTVとして張ったままだと
        // 同一リソースのRTV/SRV同時バインドになってしまう
        //----------------------------------------------------------
        ID3D11RenderTargetView* nullRTV = nullptr;
        context->OMSetRenderTargets(1, &nullRTV, nullptr);

        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthDefault(), 0);
    }

    //------------------------------------------------------------
    //! @brief IBL irradiance畳み込みパスの実行
    //------------------------------------------------------------
    void Renderer::ExecuteIBLIrradiancePass() {
        if(!m_iblIrradianceCube || !m_iblIrradianceCube->IsValid() || !m_iblCaptureCube || !m_tonemapVS || !m_iblIrradiancePS)
            return;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        context->VSSetShader(m_tonemapVS.Get(), nullptr, 0);
        context->PSSetShader(m_iblIrradiancePS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

        D3D11_VIEWPORT vp{};
        vp.Width    = static_cast<float>(kIBLIrradianceSize);
        vp.Height   = static_cast<float>(kIBLIrradianceSize);
        vp.MaxDepth = 1.0f;
        context->RSSetViewports(1, &vp);

        // キャプチャキューブ (t20) とサンプラー (s10)
        constexpr UINT captureSRVSlot = static_cast<UINT>(SRVSlot::IBLCaptureSource);
        constexpr UINT bakeSamplerSlot = static_cast<UINT>(SamplerSlot::IBL);
        ID3D11ShaderResourceView* captureSRV = m_iblCaptureCube->GetSRV();
        ID3D11SamplerState*       linearClamp = m_resources.GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp);
        context->PSSetShaderResources(captureSRVSlot, 1, &captureSRV);
        context->PSSetSamplers(bakeSamplerSlot, 1, &linearClamp);

        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for(u32 face = 0; face < 6; ++face) {
            CBufferScene faceScene = BuildCubeFaceSceneData(face);
            context->UpdateSubresource(m_sceneBuffer.Get(), 0, nullptr, &faceScene, 0, 0);
            context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_sceneBuffer.GetAddressOf());

            ID3D11RenderTargetView* rtv = m_iblIrradianceCube->GetFaceRTV(face, 0);
            context->OMSetRenderTargets(1, &rtv, nullptr);

            context->Draw(3, 0);
        }

        //----------------------------------------------------------
        // RTVバインドを解除する。このフレーム後半のLighting/Worldパスが
        // このirradianceキューブをSRV（t17）として読むため
        //----------------------------------------------------------
        ID3D11RenderTargetView* nullRTV = nullptr;
        context->OMSetRenderTargets(1, &nullRTV, nullptr);

        // t20（キャプチャキューブ）はスペキュラプレフィルタパスでも同じソースを読むので
        // ここでは解除しない（次のパスで同じSRVを張り直すだけなので害がない）
    }

    //------------------------------------------------------------
    //! @brief IBLスペキュラプレフィルタパスの実行（面×mipごとに1回）
    //------------------------------------------------------------
    void Renderer::ExecuteIBLSpecularPrefilterPass() {
        if(!m_iblPrefilteredSpecularCube || !m_iblPrefilteredSpecularCube->IsValid() || !m_iblCaptureCube || !m_tonemapVS
           || !m_iblSpecularPrefilterPS)
            return;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        context->VSSetShader(m_tonemapVS.Get(), nullptr, 0);
        context->PSSetShader(m_iblSpecularPrefilterPS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

        constexpr UINT captureSRVSlot   = static_cast<UINT>(SRVSlot::IBLCaptureSource);
        constexpr UINT bakeSamplerSlot  = static_cast<UINT>(SamplerSlot::IBL);
        ID3D11ShaderResourceView* captureSRV  = m_iblCaptureCube->GetSRV();
        ID3D11SamplerState*       linearClamp = m_resources.GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp);
        context->PSSetShaderResources(captureSRVSlot, 1, &captureSRV);
        context->PSSetSamplers(bakeSamplerSlot, 1, &linearClamp);

        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        const u32 mipCount = m_iblPrefilteredSpecularCube->GetMipLevels();

        // 一発ベイクなのでサンプル数はケチらない。mipが荒い（ラフネスが高い）ほど
        // GGXローブが広がりノイズが出やすいため、mipに応じて増やす
        static const u32 kSampleCountPerMip[6] = {32, 64, 96, 128, 192, 256};

        for(u32 mip = 0; mip < mipCount; ++mip) {
            CBufferIBLBake bakeData{};
            bakeData.roughness   = (mipCount > 1) ? (static_cast<float>(mip) / static_cast<float>(mipCount - 1)) : 0.0f;
            bakeData.sampleCount = kSampleCountPerMip[(mip < 6) ? mip : 5];

            context->UpdateSubresource(m_iblBakeBuffer.Get(), 0, nullptr, &bakeData, 0, 0);
            context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::IBLBake), 1, m_iblBakeBuffer.GetAddressOf());

            const u32 mipSize = m_iblPrefilteredSpecularCube->GetMipSize(mip);
            D3D11_VIEWPORT vp{};
            vp.Width    = static_cast<float>(mipSize);
            vp.Height   = static_cast<float>(mipSize);
            vp.MaxDepth = 1.0f;
            context->RSSetViewports(1, &vp);

            for(u32 face = 0; face < 6; ++face) {
                CBufferScene faceScene = BuildCubeFaceSceneData(face);
                context->UpdateSubresource(m_sceneBuffer.Get(), 0, nullptr, &faceScene, 0, 0);
                context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_sceneBuffer.GetAddressOf());

                ID3D11RenderTargetView* rtv = m_iblPrefilteredSpecularCube->GetFaceRTV(face, mip);
                context->OMSetRenderTargets(1, &rtv, nullptr);

                context->Draw(3, 0);
            }
        }

        //----------------------------------------------------------
        // RTV・t20（キャプチャキューブ）の解除。
        // このフレーム後半のLighting/Worldパスがプレフィルタ済みキューブを
        // SRV（t18）として読むため、RTVを張ったままにしない
        //----------------------------------------------------------
        ID3D11RenderTargetView* nullRTV = nullptr;
        context->OMSetRenderTargets(1, &nullRTV, nullptr);

        ID3D11ShaderResourceView* nullCaptureSRV = nullptr;
        context->PSSetShaderResources(captureSRVSlot, 1, &nullCaptureSRV);
    }

    //------------------------------------------------------------
    //! @brief IBL BRDF LUT生成パスの実行（起動時に1回だけ呼ぶ）
    //------------------------------------------------------------
    void Renderer::ExecuteIBLBRDFLUTPass() {
        if(!m_iblBRDFLUTRTV || !m_tonemapVS || !m_iblBRDFLUTPS)
            return;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        context->VSSetShader(m_tonemapVS.Get(), nullptr, 0);
        context->PSSetShader(m_iblBRDFLUTPS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

        D3D11_VIEWPORT vp{};
        vp.Width    = static_cast<float>(kIBLBRDFLUTSize);
        vp.Height   = static_cast<float>(kIBLBRDFLUTSize);
        vp.MaxDepth = 1.0f;
        context->RSSetViewports(1, &vp);

        ID3D11RenderTargetView* rtv = m_iblBRDFLUTRTV.Get();
        context->OMSetRenderTargets(1, &rtv, nullptr);

        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->Draw(3, 0);

        // RTVバインドを解除する（定常フレームでこのLUTをSRV(t19)として読むため）
        ID3D11RenderTargetView* nullRTV = nullptr;
        context->OMSetRenderTargets(1, &nullRTV, nullptr);

        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthDefault(), 0);
    }

    //------------------------------------------------------------
    //! @brief IBL消費側向けにb10, t17〜t19, s10をバインドする
    //------------------------------------------------------------
    void Renderer::BindIBLResources() {
        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::IBL), 1, m_iblBuffer.GetAddressOf());

        ID3D11ShaderResourceView* iblSRVs[3] = {
            m_iblIrradianceCube ? m_iblIrradianceCube->GetSRV() : nullptr,
            m_iblPrefilteredSpecularCube ? m_iblPrefilteredSpecularCube->GetSRV() : nullptr,
            m_iblBRDFLUTSRV.Get(),
        };
        constexpr UINT iblSRVSlot = static_cast<UINT>(SRVSlot::IBLIrradiance);
        context->PSSetShaderResources(iblSRVSlot, 3, iblSRVs);

        ID3D11SamplerState* linearClamp = m_resources.GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp);
        context->PSSetSamplers(static_cast<UINT>(SamplerSlot::IBL), 1, &linearClamp);
    }

    //------------------------------------------------------------
    //! @brief BindIBLResourcesで張ったSRVを解除する
    //------------------------------------------------------------
    void Renderer::UnbindIBLResources() {
        ID3D11DeviceContext*      context      = m_graphicsContext.GetContext();
        ID3D11ShaderResourceView* nullSRVs[3]  = {nullptr, nullptr, nullptr};
        constexpr UINT            iblSRVSlot   = static_cast<UINT>(SRVSlot::IBLIrradiance);
        context->PSSetShaderResources(iblSRVSlot, 3, nullSRVs);
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
        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

        //----------------------------------------------------------
        // Scene (b0) をバインド
        //----------------------------------------------------------
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_sceneBuffer.GetAddressOf());
        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_sceneBuffer.GetAddressOf());

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
        ID3D11SamplerState* pointClamp = m_resources.GetSampler(Tsukino::GraphicsCommon::SamplerType::PointClamp);
        constexpr UINT      gbufferSamplerSlot = static_cast<UINT>(SamplerSlot::GBuffer);
        context->PSSetSamplers(gbufferSamplerSlot, 1, &pointClamp);

        //----------------------------------------------------------
        // IBL (b10, t17〜t19, s10)：スカイ由来のアンビエント
        //----------------------------------------------------------
        BindIBLResources();

        //----------------------------------------------------------
        // フルスクリーントライアングル描画
        //----------------------------------------------------------
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->Draw(3, 0);

        //----------------------------------------------------------
        // 後片付け：G-Buffer/深度/シャドウマップ/IBLのSRVを解除
        // （直後にDSVとして再バインドする深度との同時バインド防止のため必須）
        //----------------------------------------------------------
        ID3D11ShaderResourceView* nullSRVs[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        context->PSSetShaderResources(gbufferSRVSlot, 6, nullSRVs);
        ID3D11ShaderResourceView* nullShadowSRV = nullptr;
        context->PSSetShaderResources(shadowSRVSlot, 1, &nullShadowSRV);
        UnbindIBLResources();

        //----------------------------------------------------------
        // 深度ステートを元に戻す（HDRRenderTarget復帰後のWorld/Transparent用）
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthDefault(), 0);
    }

    //------------------------------------------------------------
    //! @brief フォグパスの実行
    //! @note  深度(t13)だけを読み、HDRバッファへプリマルチプライのover合成で
    //!        書き込む。HDRをSRVとして読まないためRTVに張ったままでよく、
    //!        ポストプロセス用中間バッファを消費しない（モーションブラーと
    //!        取り合いにならない）。
    //------------------------------------------------------------
    void Renderer::ExecuteFogPass() {
        if(!m_fogEnabled || !m_hasFog || !m_tonemapVS || !m_fogPS)
            return;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        //----------------------------------------------------------
        // HDRのみをRTVにバインド（深度をSRVとして読むためDSVは外す）
        //----------------------------------------------------------
        m_graphicsContext.BindHDRTargetOnly();

        //----------------------------------------------------------
        // 深度(t13)とポイントサンプラー(s9)をバインド
        //----------------------------------------------------------
        constexpr UINT            depthSRVSlot = static_cast<UINT>(SRVSlot::GBufferDepth);
        ID3D11ShaderResourceView* depthSRV     = m_graphicsContext.GetDepthSRV();
        context->PSSetShaderResources(depthSRVSlot, 1, &depthSRV);

        ID3D11SamplerState* pointClamp         = m_resources.GetSampler(Tsukino::GraphicsCommon::SamplerType::PointClamp);
        constexpr UINT      gbufferSamplerSlot = static_cast<UINT>(SamplerSlot::GBuffer);
        context->PSSetSamplers(gbufferSamplerSlot, 1, &pointClamp);

        //----------------------------------------------------------
        // Scene (b0) をバインド（invViewProj・cameraPos・lightDirを使う）
        //----------------------------------------------------------
        constexpr UINT sceneCBSlot = static_cast<UINT>(CBSlot::Scene);
        context->PSSetConstantBuffers(sceneCBSlot, 1, m_sceneBuffer.GetAddressOf());

        //----------------------------------------------------------
        // パラメータ (b9) を更新してバインド
        //----------------------------------------------------------
        context->UpdateSubresource(m_fogBuffer.Get(), 0, nullptr, &m_fogData, 0, 0);
        constexpr UINT fogCBSlot = static_cast<UINT>(CBSlot::Fog);
        context->PSSetConstantBuffers(fogCBSlot, 1, m_fogBuffer.GetAddressOf());

        //----------------------------------------------------------
        // シェーダーをセット（VSはTonemapと共用のフルスクリーン三角形用）
        //----------------------------------------------------------
        context->VSSetShader(m_tonemapVS.Get(), nullptr, 0);
        context->PSSetShader(m_fogPS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        //----------------------------------------------------------
        // 深度なし・プリマルチプライアルファでover合成
        // （DirectXTKのAlphaBlendは ONE / INV_SRC_ALPHA なので、
        //   PSがfloat4(color * f, f)を返せばそのまま正しいoverになる）
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->AlphaBlend(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

        //----------------------------------------------------------
        // フルスクリーントライアングル描画
        //----------------------------------------------------------
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->Draw(3, 0);

        //----------------------------------------------------------
        // 後片付け：深度を次フレームDSVとして再バインドするため、
        // SRVのバインドを必ず解除する（ExecuteLightingPassと同じ理由）。
        // ブレンドも不透明へ戻しておく
        //----------------------------------------------------------
        ID3D11ShaderResourceView* nullSRV = nullptr;
        context->PSSetShaderResources(depthSRVSlot, 1, &nullSRV);

        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
    }

    //------------------------------------------------------------
    //! 環境パーティクルパスを実行します。
    //------------------------------------------------------------
    void Renderer::ExecuteAmbientParticlePass() {
        if(!m_ambientParticleEnabled || !m_hasAmbientParticle || m_ambientParticleCount == 0)
            return;

        ID3D11DeviceContext* context = m_graphicsContext.GetContext();

        //----------------------------------------------------------
        // レンダーターゲットは切り替えない。
        // Render()のBindHDRRenderTarget()で貼ったHDR＋DSVがここまで生きており
        // （World/TransparentDepth/Transparentはターゲットを触らない）、
        // 深度をSRVとして読むこともないので同時バインドの問題も起きない
        //----------------------------------------------------------

        //----------------------------------------------------------
        // Scene (b0) をバインド（view・viewProj・cameraPosを頂点シェーダーで使う）
        //----------------------------------------------------------
        constexpr UINT sceneCBSlot = static_cast<UINT>(CBSlot::Scene);
        context->VSSetConstantBuffers(sceneCBSlot, 1, m_sceneBuffer.GetAddressOf());

        //----------------------------------------------------------
        // パラメータ (b10) を更新してバインド（頂点シェーダー専用）
        //----------------------------------------------------------
        context->UpdateSubresource(m_ambientParticleBuffer.Get(), 0, nullptr, &m_ambientParticleData, 0, 0);
        constexpr UINT particleCBSlot = static_cast<UINT>(CBSlot::AmbientParticle);
        context->VSSetConstantBuffers(particleCBSlot, 1, m_ambientParticleBuffer.GetAddressOf());

        //----------------------------------------------------------
        // シェーダーをセット（頂点バッファが無いので入力レイアウトも不要）
        //----------------------------------------------------------
        context->VSSetShader(m_ambientParticleVS.Get(), nullptr, 0);
        context->PSSetShader(m_ambientParticlePS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        //----------------------------------------------------------
        // 深度テストあり・深度書き込みなし・加算合成
        //
        // DepthReadReverseZはGREATER_EQUALなのでこのエンジンのリバースZと
        // 一致する（DepthReadはLESS_EQUALなので使ってはいけない）。
        // 深度を書かないことで粒子どうしの前後関係を気にせずに済み、
        // 加算合成なので描画順のソートも不要になる
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthReadReverseZ(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Additive(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

        //----------------------------------------------------------
        // 頂点バッファなしで「1粒 = 三角形2枚」を一括描画
        //----------------------------------------------------------
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->Draw(m_ambientParticleCount * 6, 0);

        //----------------------------------------------------------
        // ステートを戻す（Sky / Fogと同じ流儀）
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthDefault(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
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

        ID3D11SamplerState* linearClamp = m_resources.GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp);
        context->PSSetSamplers(0, 1, &linearClamp);

        constexpr UINT            velocitySRVSlot = static_cast<UINT>(SRVSlot::GBufferVelocity);
        ID3D11ShaderResourceView* velocitySRV     = m_graphicsContext.GetGBufferSRV(5);
        context->PSSetShaderResources(velocitySRVSlot, 1, &velocitySRV);

        ID3D11SamplerState* pointClamp = m_resources.GetSampler(Tsukino::GraphicsCommon::SamplerType::PointClamp);
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
        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

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
        ID3D11SamplerState* sampler = m_resources.GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp);
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
        context->OMSetDepthStencilState(m_resources.GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources.GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources.GetCommonStatesTK()->CullNone());

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

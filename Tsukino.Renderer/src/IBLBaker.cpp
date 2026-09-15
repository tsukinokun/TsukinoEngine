//----------------------------------------------------------------------------
//! @file   IBLBaker.cpp
//! @brief  スカイ由来の環境光（IBL）のベイクの実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/IBLBaker.hpp>

#include "FullscreenPass.hpp"

#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>
#include <Tsukino/Renderer/RenderResources.hpp>
#include <Tsukino/Renderer/FrameConstants.hpp>
#include <Tsukino/Renderer/SkyPass.hpp>
#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 定数バッファ（b10 と、ベイク中だけ使う b11）を作成します。
    //------------------------------------------------------------------------
    bool IBLBaker::CreateConstantBuffers(ID3D11Device* device) {
        D3D11_BUFFER_DESC desc = {};
        desc.Usage             = D3D11_USAGE_DEFAULT;
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;

        // CBufferIBL (b10)
        desc.ByteWidth = sizeof(CBufferIBL);
        if(FAILED(device->CreateBuffer(&desc, nullptr, m_buffer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create IBL constant buffer.");
            return false;
        }

        // CBufferIBLBake (b11)：IBLベイク（キャプチャ/irradiance畳み込み/スペキュラプレフィルタ）実行中だけ使う一時バッファ
        desc.ByteWidth = sizeof(CBufferIBLBake);
        if(FAILED(device->CreateBuffer(&desc, nullptr, m_bakeBuffer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create IBL bake constant buffer.");
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! キューブマップ・BRDF LUT・ベイク用シェーダーを作成し、BRDF LUT を焼きます。
    //------------------------------------------------------------------------
    bool IBLBaker::Initialize(GraphicsContext&                   graphicsContext,
                              RenderResources&                   resources,
                              FrameConstants&                    frameConstants,
                              const FullscreenPass&              fullscreenPass,
                              const SkyPass&                     skyPass,
                              const Tsukino::Asset::ShaderAsset* irradiancePS,
                              const Tsukino::Asset::ShaderAsset* specularPrefilterPS,
                              const Tsukino::Asset::ShaderAsset* brdfLutPS) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;
        m_fullscreenPass  = &fullscreenPass;
        m_skyPass         = &skyPass;

        ID3D11Device* device = graphicsContext.GetDevice();

        constexpr DXGI_FORMAT kCubeFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;    // HDR：sunIntensityは1.0を大きく超える

        //------------------------------------------------------------
        // キューブマップ本体3枚
        //------------------------------------------------------------
        m_captureCube             = std::make_unique<DX11TextureCube>(kCaptureSize, 1, kCubeFormat, device);
        m_irradianceCube          = std::make_unique<DX11TextureCube>(kIrradianceSize, 1, kCubeFormat, device);
        m_prefilteredSpecularCube = std::make_unique<DX11TextureCube>(kSpecularBaseSize, kSpecularMipCount, kCubeFormat, device);

        if(!m_captureCube->IsValid() || !m_irradianceCube->IsValid() || !m_prefilteredSpecularCube->IsValid()) {
            Tsukino::Core::Log::Error("Renderer: Failed to create one or more IBL cube maps.");
            return false;
        }

        //------------------------------------------------------------
        // BRDF LUT（スカイに依存しない2D、r: スケールA, g: バイアスB）
        //------------------------------------------------------------
        D3D11_TEXTURE2D_DESC lutDesc = {};
        lutDesc.Width                = kBRDFLUTSize;
        lutDesc.Height               = kBRDFLUTSize;
        lutDesc.MipLevels            = 1;
        lutDesc.ArraySize            = 1;
        lutDesc.Format               = DXGI_FORMAT_R16G16_FLOAT;
        lutDesc.SampleDesc.Count     = 1;
        lutDesc.Usage                = D3D11_USAGE_DEFAULT;
        lutDesc.BindFlags            = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        HRESULT hr = device->CreateTexture2D(&lutDesc, nullptr, m_brdfLUTTex.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Renderer: Failed to create IBL BRDF LUT texture.");
            return false;
        }

        hr = device->CreateRenderTargetView(m_brdfLUTTex.Get(), nullptr, m_brdfLUTRTV.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Renderer: Failed to create IBL BRDF LUT RTV.");
            return false;
        }

        hr = device->CreateShaderResourceView(m_brdfLUTTex.Get(), nullptr, m_brdfLUTSRV.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Renderer: Failed to create IBL BRDF LUT SRV.");
            return false;
        }

        //------------------------------------------------------------
        // ベイク用PS（VSはすべてフルスクリーン三角形用VS/スカイのVSを共用するのでここでは作らない）
        //------------------------------------------------------------
        if(!irradiancePS || !specularPrefilterPS || !brdfLutPS) {
            Tsukino::Core::Log::Error("IBLBaker::Initialize - shader is null.");
            return false;
        }

        hr = device->CreatePixelShader(irradiancePS->binary.data(), irradiancePS->binary.size(), nullptr,
                                       m_irradiancePS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Renderer: Failed to create IBL irradiance pixel shader.");
            return false;
        }

        hr = device->CreatePixelShader(specularPrefilterPS->binary.data(), specularPrefilterPS->binary.size(),
                                       nullptr, m_specularPrefilterPS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Renderer: Failed to create IBL specular prefilter pixel shader.");
            return false;
        }

        hr = device->CreatePixelShader(brdfLutPS->binary.data(), brdfLutPS->binary.size(), nullptr,
                                       m_brdfLUTPS.GetAddressOf());
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Renderer: Failed to create IBL BRDF LUT pixel shader.");
            return false;
        }

        m_hasBakeShaders = true;

        //------------------------------------------------------------
        // 定数バッファの初期値（プレフィルタ済みスペキュラのミップ数はここで確定する）
        //------------------------------------------------------------
        m_data.specularMipCount = static_cast<float>(m_prefilteredSpecularCube->GetMipLevels());
        m_data.iblIntensity     = 1.0f;
        m_graphicsContext->GetContext()->UpdateSubresource(m_buffer.Get(), 0, nullptr, &m_data, 0, 0);

        //------------------------------------------------------------
        // BRDF LUTはスカイに依存しないため、ここで即座に一度だけベイクする
        // （キャプチャ/irradiance/プレフィルタはスカイパイプライン確立後にRender()側でトリガーされる）
        //------------------------------------------------------------
        ExecuteBRDFLUTPass();

        return true;
    }

    //------------------------------------------------------------------------
    //! まだ焼いていなければベイクします（キャプチャ → irradiance → スペキュラプレフィルタ）。
    //------------------------------------------------------------------------
    bool IBLBaker::BakeIfNeeded() {
        if(!m_skyPass->IsReady() || !m_hasBakeShaders || m_baked)
            return false;

        ExecuteCapturePass();
        ExecuteIrradiancePass();
        ExecuteSpecularPrefilterPass();
        m_baked = true;

        //--------------------------------------------------------------------
        // ビューポートを画面サイズへ戻す。
        // BeginGBufferPass()はRTV/DSVの張り替えとクリアだけでビューポートには
        // 触れないため、ここで戻しておかないと直後のGBufferパスが
        // IBLベイク最後の面（32px/4px等）のままの極小ビューポートで描かれてしまう
        //--------------------------------------------------------------------
        D3D11_VIEWPORT vp{};
        vp.Width    = static_cast<float>(m_graphicsContext->GetWidth());
        vp.Height   = static_cast<float>(m_graphicsContext->GetHeight());
        vp.MaxDepth = 1.0f;
        m_graphicsContext->GetContext()->RSSetViewports(1, &vp);

        return true;
    }

    //------------------------------------------------------------------------
    //! IBL の結果（b10, t17〜t19, s10）をピクセルシェーダーへバインドします。
    //------------------------------------------------------------------------
    void IBLBaker::Bind() {
        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::IBL), 1, m_buffer.GetAddressOf());

        ID3D11ShaderResourceView* iblSRVs[3] = {
            m_irradianceCube ? m_irradianceCube->GetSRV() : nullptr,
            m_prefilteredSpecularCube ? m_prefilteredSpecularCube->GetSRV() : nullptr,
            m_brdfLUTSRV.Get(),
        };
        constexpr UINT iblSRVSlot = static_cast<UINT>(SRVSlot::IBLIrradiance);
        context->PSSetShaderResources(iblSRVSlot, 3, iblSRVs);

        ID3D11SamplerState* linearClamp = m_resources->GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp);
        context->PSSetSamplers(static_cast<UINT>(SamplerSlot::IBL), 1, &linearClamp);
    }

    //------------------------------------------------------------------------
    //! Bind() で張った SRV を解除します。
    //------------------------------------------------------------------------
    void IBLBaker::Unbind() {
        ID3D11DeviceContext*      context      = m_graphicsContext->GetContext();
        ID3D11ShaderResourceView* nullSRVs[3]  = {nullptr, nullptr, nullptr};
        constexpr UINT            iblSRVSlot   = static_cast<UINT>(SRVSlot::IBLIrradiance);
        context->PSSetShaderResources(iblSRVSlot, 3, nullSRVs);
    }

    //------------------------------------------------------------------------
    //! 原点から見たキューブの1面ぶんのシーン定数を組み立てます（D3D11のキューブ面の並び +X,-X,+Y,-Y,+Z,-Z に合わせてあります）。
    //------------------------------------------------------------------------
    CBufferScene IBLBaker::BuildCubeFaceSceneData(u32 face) const {
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
        CBufferScene data = m_frameConstants->GetWorldSceneData();

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

    //------------------------------------------------------------------------
    //! スカイを6面のキューブマップへ焼きます（Sky.ps.hlsl を面ごとに視点だけ差し替えて使います）。
    //------------------------------------------------------------------------
    void IBLBaker::ExecuteCapturePass() {
        if(!m_captureCube || !m_captureCube->IsValid() || !m_skyPass->GetVertexShader() || !m_skyPass->GetPixelShader())
            return;

        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        context->VSSetShader(m_skyPass->GetVertexShader(), nullptr, 0);
        context->PSSetShader(m_skyPass->GetPixelShader(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        // 深度バッファを持たないオフスクリーンキャプチャなので深度テストなし
        context->OMSetDepthStencilState(m_resources->GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources->GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources->GetCommonStatesTK()->CullNone());

        D3D11_VIEWPORT vp{};
        vp.Width    = static_cast<float>(kCaptureSize);
        vp.Height   = static_cast<float>(kCaptureSize);
        vp.MaxDepth = 1.0f;
        context->RSSetViewports(1, &vp);

        // Sky (b4) は直前のExecuteSkyPass()で今フレーム分がGPUへ転送済みなのでそのままバインドする
        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Sky), 1, m_skyPass->GetBufferAddress());

        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for(u32 face = 0; face < 6; ++face) {
            CBufferScene faceScene = BuildCubeFaceSceneData(face);
            context->UpdateSubresource(m_frameConstants->GetSceneBuffer(), 0, nullptr, &faceScene, 0, 0);
            context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_frameConstants->GetSceneBufferAddress());

            ID3D11RenderTargetView* rtv = m_captureCube->GetFaceRTV(face, 0);
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

        context->OMSetDepthStencilState(m_resources->GetCommonStatesTK()->DepthDefault(), 0);
    }

    //------------------------------------------------------------------------
    //! 拡散IBL用の irradiance を畳み込みます（面ごとに1回）。
    //------------------------------------------------------------------------
    void IBLBaker::ExecuteIrradiancePass() {
        if(!m_irradianceCube || !m_irradianceCube->IsValid() || !m_captureCube || !m_fullscreenPass->IsValid() || !m_irradiancePS)
            return;

        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        context->VSSetShader(m_fullscreenPass->GetVertexShader(), nullptr, 0);
        context->PSSetShader(m_irradiancePS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        context->OMSetDepthStencilState(m_resources->GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources->GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources->GetCommonStatesTK()->CullNone());

        D3D11_VIEWPORT vp{};
        vp.Width    = static_cast<float>(kIrradianceSize);
        vp.Height   = static_cast<float>(kIrradianceSize);
        vp.MaxDepth = 1.0f;
        context->RSSetViewports(1, &vp);

        // キャプチャキューブ (t20) とサンプラー (s10)
        constexpr UINT captureSRVSlot = static_cast<UINT>(SRVSlot::IBLCaptureSource);
        constexpr UINT bakeSamplerSlot = static_cast<UINT>(SamplerSlot::IBL);
        ID3D11ShaderResourceView* captureSRV = m_captureCube->GetSRV();
        ID3D11SamplerState*       linearClamp = m_resources->GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp);
        context->PSSetShaderResources(captureSRVSlot, 1, &captureSRV);
        context->PSSetSamplers(bakeSamplerSlot, 1, &linearClamp);

        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for(u32 face = 0; face < 6; ++face) {
            CBufferScene faceScene = BuildCubeFaceSceneData(face);
            context->UpdateSubresource(m_frameConstants->GetSceneBuffer(), 0, nullptr, &faceScene, 0, 0);
            context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_frameConstants->GetSceneBufferAddress());

            ID3D11RenderTargetView* rtv = m_irradianceCube->GetFaceRTV(face, 0);
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

    //------------------------------------------------------------------------
    //! 鏡面IBL用のスペキュラをプレフィルタします（面×mipごとに1回）。
    //------------------------------------------------------------------------
    void IBLBaker::ExecuteSpecularPrefilterPass() {
        if(!m_prefilteredSpecularCube || !m_prefilteredSpecularCube->IsValid() || !m_captureCube || !m_fullscreenPass->IsValid()
           || !m_specularPrefilterPS)
            return;

        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        context->VSSetShader(m_fullscreenPass->GetVertexShader(), nullptr, 0);
        context->PSSetShader(m_specularPrefilterPS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        context->OMSetDepthStencilState(m_resources->GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources->GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources->GetCommonStatesTK()->CullNone());

        constexpr UINT captureSRVSlot   = static_cast<UINT>(SRVSlot::IBLCaptureSource);
        constexpr UINT bakeSamplerSlot  = static_cast<UINT>(SamplerSlot::IBL);
        ID3D11ShaderResourceView* captureSRV  = m_captureCube->GetSRV();
        ID3D11SamplerState*       linearClamp = m_resources->GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp);
        context->PSSetShaderResources(captureSRVSlot, 1, &captureSRV);
        context->PSSetSamplers(bakeSamplerSlot, 1, &linearClamp);

        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        const u32 mipCount = m_prefilteredSpecularCube->GetMipLevels();

        // 一発ベイクなのでサンプル数はケチらない。mipが荒い（ラフネスが高い）ほど
        // GGXローブが広がりノイズが出やすいため、mipに応じて増やす
        static const u32 kSampleCountPerMip[6] = {32, 64, 96, 128, 192, 256};

        for(u32 mip = 0; mip < mipCount; ++mip) {
            CBufferIBLBake bakeData{};
            bakeData.roughness   = (mipCount > 1) ? (static_cast<float>(mip) / static_cast<float>(mipCount - 1)) : 0.0f;
            bakeData.sampleCount = kSampleCountPerMip[(mip < 6) ? mip : 5];

            context->UpdateSubresource(m_bakeBuffer.Get(), 0, nullptr, &bakeData, 0, 0);
            context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::IBLBake), 1, m_bakeBuffer.GetAddressOf());

            const u32 mipSize = m_prefilteredSpecularCube->GetMipSize(mip);
            D3D11_VIEWPORT vp{};
            vp.Width    = static_cast<float>(mipSize);
            vp.Height   = static_cast<float>(mipSize);
            vp.MaxDepth = 1.0f;
            context->RSSetViewports(1, &vp);

            for(u32 face = 0; face < 6; ++face) {
                CBufferScene faceScene = BuildCubeFaceSceneData(face);
                context->UpdateSubresource(m_frameConstants->GetSceneBuffer(), 0, nullptr, &faceScene, 0, 0);
                context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_frameConstants->GetSceneBufferAddress());

                ID3D11RenderTargetView* rtv = m_prefilteredSpecularCube->GetFaceRTV(face, mip);
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

    //------------------------------------------------------------------------
    //! BRDF LUT を生成します（スカイに依存しないため起動時に1回だけ呼ぶ）。
    //------------------------------------------------------------------------
    void IBLBaker::ExecuteBRDFLUTPass() {
        if(!m_brdfLUTRTV || !m_fullscreenPass->IsValid() || !m_brdfLUTPS)
            return;

        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        context->VSSetShader(m_fullscreenPass->GetVertexShader(), nullptr, 0);
        context->PSSetShader(m_brdfLUTPS.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        context->OMSetDepthStencilState(m_resources->GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources->GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources->GetCommonStatesTK()->CullNone());

        D3D11_VIEWPORT vp{};
        vp.Width    = static_cast<float>(kBRDFLUTSize);
        vp.Height   = static_cast<float>(kBRDFLUTSize);
        vp.MaxDepth = 1.0f;
        context->RSSetViewports(1, &vp);

        ID3D11RenderTargetView* rtv = m_brdfLUTRTV.Get();
        context->OMSetRenderTargets(1, &rtv, nullptr);

        FullscreenPass::Draw(context);

        // RTVバインドを解除する（定常フレームでこのLUTをSRV(t19)として読むため）
        ID3D11RenderTargetView* nullRTV = nullptr;
        context->OMSetRenderTargets(1, &nullRTV, nullptr);

        context->OMSetDepthStencilState(m_resources->GetCommonStatesTK()->DepthDefault(), 0);
    }
}    // namespace Tsukino::Renderer

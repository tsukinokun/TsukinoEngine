//----------------------------------------------------------------------------
//! @file   ShadowPass.cpp
//! @brief  シャドウマップパスの実装
//----------------------------------------------------------------------------
#include "ShadowPass.hpp"
#include "DrawCommandExecutor.hpp"

#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>

#include <cmath>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! シャドウマップと影用パイプラインを作成します。
    //------------------------------------------------------------------------
    bool ShadowPass::Initialize(GraphicsContext&                   graphicsContext,
                                RenderResources&                   resources,
                                FrameConstants&                    frameConstants,
                                DrawCommandExecutor&               commandExecutor,
                                const Tsukino::Asset::ShaderAsset* staticVS,
                                const Tsukino::Asset::ShaderAsset* skeletalVS,
                                const Tsukino::Asset::ShaderAsset* ps) {
        m_graphicsContext = &graphicsContext;
        m_frameConstants  = &frameConstants;
        m_commandExecutor = &commandExecutor;

        PipelineFactory* factory = resources.GetPipelineFactory();
        if(!factory)
            return false;

        // 静的メッシュ用シャドウパイプライン
        if(staticVS && ps) {
            m_staticPipeline = factory->Create(*staticVS, *ps, Tsukino::GraphicsCommon::VertexFormat::PositionNormalUV, DepthMode::ReadWrite);
            if(!m_staticPipeline) {
                Tsukino::Core::Log::Error("Renderer: Shadow Static Pipeline generation failed.");
                return false;
            }
        }

        // スキニングメッシュ用シャドウパイプライン
        if(skeletalVS && ps) {
            m_skeletalPipeline = factory->Create(*skeletalVS, *ps, Tsukino::GraphicsCommon::VertexFormat::Skinned, DepthMode::ReadWrite);
            if(!m_skeletalPipeline) {
                Tsukino::Core::Log::Error("Renderer: Shadow Skeletal Pipeline generation failed.");
                return false;
            }
        }

        return CreateShadowMap(graphicsContext.GetDevice());
    }

    //------------------------------------------------------------------------
    //! GBuffer パスの描画コマンドのうち、影を落とすものをシャドウマップへ描きます。
    //------------------------------------------------------------------------
    bool ShadowPass::Execute(const std::vector<DrawCommand>& commands, Renderer::FrameStats& stats) {
        if(!m_staticPipeline && !m_skeletalPipeline)
            return false;

        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        // シャドウマップをクリア
        context->ClearDepthStencilView(m_mapDSV.Get(), D3D11_CLEAR_DEPTH, 0.0f, 0);

        // カラーRTをnullにしてDSVだけセット
        ID3D11RenderTargetView* nullRTV = nullptr;
        context->OMSetRenderTargets(1, &nullRTV, m_mapDSV.Get());

        // シャドウマップ解像度でビューポートをセット
        D3D11_VIEWPORT vp{};
        vp.Width    = static_cast<float>(kMapSize);
        vp.Height   = static_cast<float>(kMapSize);
        vp.MaxDepth = 1.0f;
        context->RSSetViewports(1, &vp);

        m_frameConstants->UploadWorld();

        for(const auto& cmd : commands) {
            if(cmd.pass != RenderPass::GBuffer)
                continue;

            // 頂点シェーダーが独自に頂点を組み立てるオブジェクトは、
            // 固定のシャドウ用シェーダーでは形を再現できないので外す
            if(!cmd.castsShadow)
                continue;

            m_commandExecutor->ExecuteShadow(cmd, m_staticPipeline.get(), m_skeletalPipeline.get(), stats);
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! シャドウマップ（t8）と比較サンプラー（s8）をピクセルシェーダーへバインドします。
    //------------------------------------------------------------------------
    void ShadowPass::BindForSampling(ID3D11DeviceContext* context) const {
        constexpr UINT srvSlot     = static_cast<UINT>(SRVSlot::ShadowMap);
        constexpr UINT samplerSlot = static_cast<UINT>(SamplerSlot::ShadowMap);
        context->PSSetShaderResources(srvSlot, 1, m_mapSRV.GetAddressOf());
        context->PSSetSamplers(samplerSlot, 1, m_sampler.GetAddressOf());
    }

    //------------------------------------------------------------------------
    //! ディレクショナルライトの ViewProjection 行列を求めます。
    //------------------------------------------------------------------------
    Tsukino::Core::Math::matrix ShadowPass::ComputeLightViewProj(const hlslpp::float3& normalizedDir, const hlslpp::float3& focusPoint) {
        //--------------------------------------------------------------------
        // ディレクショナルライトは平行投影を使う。
        //
        // シャドウの投影範囲は呼び出し側から渡されたfocusPoint（通常はメインカメラの
        // 注視点）を中心にする（ワールド原点固定だと、カメラが原点から離れる
        // プレイヤー追従型のシーンで影が一切映らなくなる）。
        // カメラ位置そのものを中心にしないのは、TPSカメラのように注視点から
        // 離れた位置にカメラを置く構成だと、画面に映る注視点付近がシャドウ範囲の
        // 端に寄ってしまい、キャラクターのすぐ近くで影が途切れて見えるため
        //--------------------------------------------------------------------
        hlslpp::float3 target = focusPoint;

        // ライトの位置はターゲットから十分離れた場所に置く
        hlslpp::float3 lightPos = target - normalizedDir * 500.0f;
        hlslpp::float3 up       = hlslpp::float3(0.0f, 1.0f, 0.0f);

        // ライト方向が真上/真下に近いときupベクトルが平行になるので回避
        float dotUp = std::abs(hlslpp::dot(normalizedDir, up));
        if(dotUp > 0.99f) {
            up = hlslpp::float3(0.0f, 0.0f, 1.0f);
        }

        // LookAt でライトのView行列、平行投影でProj行列を作成
        Tsukino::Core::Math::matrix lightView = Tsukino::Core::Math::matrix::lookAtLH(lightPos, target, up);
        Tsukino::Core::Math::matrix lightProj = Tsukino::Core::Math::matrix::orthographicOffCenterLH(-500.0f,    // left
                                                                                                     500.0f,     // right
                                                                                                     -500.0f,    // bottom
                                                                                                     500.0f,     // top
                                                                                                     2000.0f,    // far
                                                                                                     1.0f        // near
        );

        return hlslpp::mul(lightView, lightProj);
    }

    //------------------------------------------------------------------------
    //! シャドウマップ（テクスチャ・DSV・SRV・比較サンプラー）を作成します。
    //------------------------------------------------------------------------
    bool ShadowPass::CreateShadowMap(ID3D11Device* device) {
        //--------------------------------------------------------------------
        // シャドウマップテクスチャの作成
        // R32_TYPELESS : DSV(D32_FLOAT)とSRV(R32_FLOAT)で共有するため
        //--------------------------------------------------------------------
        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width            = kMapSize;
        texDesc.Height           = kMapSize;
        texDesc.MipLevels        = 1;
        texDesc.ArraySize        = 1;
        texDesc.Format           = DXGI_FORMAT_R32_TYPELESS;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage            = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

        if(FAILED(device->CreateTexture2D(&texDesc, nullptr, m_mapTex.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create shadow map texture.");
            return false;
        }

        // DSVの作成（深度書き込み用）
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format        = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

        if(FAILED(device->CreateDepthStencilView(m_mapTex.Get(), &dsvDesc, m_mapDSV.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create shadow map DSV.");
            return false;
        }

        // SRVの作成（PSでのサンプリング用）
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels       = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        if(FAILED(device->CreateShaderResourceView(m_mapTex.Get(), &srvDesc, m_mapSRV.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create shadow map SRV.");
            return false;
        }

        //--------------------------------------------------------------------
        // PCF用比較サンプラーの作成
        // SampleCmpLevelZero で使用する
        //--------------------------------------------------------------------
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

        if(FAILED(device->CreateSamplerState(&samplerDesc, m_sampler.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create shadow sampler.");
            return false;
        }

        return true;
    }
}    // namespace Tsukino::Renderer

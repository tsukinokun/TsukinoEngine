//----------------------------------------------------------------------------
//! @file   LightingPass.cpp
//! @brief  ディファードライティングパスの実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/LightingPass.hpp>

#include "FullscreenPass.hpp"
#include "ShadowPass.hpp"

#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>
#include <Tsukino/Renderer/RenderResources.hpp>
#include <Tsukino/Renderer/FrameConstants.hpp>
#include <Tsukino/Renderer/IBLBaker.hpp>
#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <cstring>
#include <string>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! ライト配列の定数バッファ（b6）とピクセルシェーダーを作成します。
    //------------------------------------------------------------------------
    bool LightingPass::Initialize(GraphicsContext&                   graphicsContext,
                                  RenderResources&                   resources,
                                  FrameConstants&                    frameConstants,
                                  const FullscreenPass&              fullscreenPass,
                                  const ShadowPass&                  shadowPass,
                                  IBLBaker&                          iblBaker,
                                  const Tsukino::Asset::ShaderAsset* ps) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;
        m_fullscreenPass  = &fullscreenPass;
        m_shadowPass      = &shadowPass;
        m_iblBaker        = &iblBaker;

        ID3D11Device* device = graphicsContext.GetDevice();

        // 点光源・スポットライト配列用の定数バッファ (b6)
        D3D11_BUFFER_DESC desc = {};
        desc.Usage             = D3D11_USAGE_DEFAULT;
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        desc.ByteWidth         = sizeof(CBufferLights);
        if(FAILED(device->CreateBuffer(&desc, nullptr, m_lightsBuffer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create lights constant buffer.");
            return false;
        }

        // ピクセルシェーダー（頂点シェーダーはフルスクリーン三角形用を共用するので作らない）
        if(!ps) {
            Tsukino::Core::Log::Error("LightingPass::Initialize - shader is null.");
            return false;
        }

        if(FAILED(device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_ps.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create lighting pixel shader.");
            m_ps.Reset();
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! ディレクショナルライトを設定します。
    //------------------------------------------------------------------------
    void LightingPass::SetDirectionalLight(const hlslpp::float3& direction, const hlslpp::float3& color, float intensity, const hlslpp::float3& focusPoint) {
        //------------------------------------------------------------
        // ライト方向を正規化
        //------------------------------------------------------------
        hlslpp::float3 normalizedDir = hlslpp::normalize(direction);

        //------------------------------------------------------------
        // ライト空間の ViewProjection（シャドウマップの投影）を求めて、
        // ワールドのシーン定数へ書き込む
        //------------------------------------------------------------
        m_frameConstants->SetDirectionalLight(ShadowPass::ComputeLightViewProj(normalizedDir, focusPoint),
                                             hlslpp::float4(normalizedDir.x, normalizedDir.y, normalizedDir.z, 0.0f),
                                             hlslpp::float4(color.x, color.y, color.z, intensity));
    }

    //------------------------------------------------------------------------
    //! 点光源・スポットライトの配列を設定します（MAX_LIGHTS を超える分は切り捨て、初回のみ警告します）。
    //------------------------------------------------------------------------
    void LightingPass::SetLights(const GPULight* lights, u32 count) {
        u32 copyCount = std::min(count, MAX_LIGHTS);

        if(count > MAX_LIGHTS && !m_lightOverflowWarned) {
            Tsukino::Core::Log::Error("LightingPass::SetLights - light count (" + std::to_string(count) + ") exceeds MAX_LIGHTS ("
                                      + std::to_string(MAX_LIGHTS) + "). Extra lights are dropped.");
            m_lightOverflowWarned = true;
        }

        m_lightsData.lightCount = copyCount;
        if(copyCount > 0) {
            std::memcpy(m_lightsData.lights, lights, sizeof(GPULight) * copyCount);
        }
    }

    //------------------------------------------------------------------------
    //! ライティングを実行して HDR バッファへ加算します。
    //------------------------------------------------------------------------
    void LightingPass::Execute() {
        if(!m_ps || !m_fullscreenPass->IsValid())
            return;

        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        //----------------------------------------------------------
        // HDRバッファのみをRTVにバインド（深度をSRVとして読むためDSVは外す）
        //----------------------------------------------------------
        m_graphicsContext->BindHDRTargetOnly();

        //----------------------------------------------------------
        // シェーダーをセット（VSはフルスクリーン三角形用を共用）
        //----------------------------------------------------------
        context->VSSetShader(m_fullscreenPass->GetVertexShader(), nullptr, 0);
        context->PSSetShader(m_ps.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        //----------------------------------------------------------
        // 深度テストなし・ブレンドなし（discardで背景ピクセルを保護する）
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_resources->GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources->GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources->GetCommonStatesTK()->CullNone());

        //----------------------------------------------------------
        // Scene (b0) をバインド
        //----------------------------------------------------------
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_frameConstants->GetSceneBufferAddress());
        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_frameConstants->GetSceneBufferAddress());

        //----------------------------------------------------------
        // Lights (b5) を更新してバインド
        //----------------------------------------------------------
        context->UpdateSubresource(m_lightsBuffer.Get(), 0, nullptr, &m_lightsData, 0, 0);
        constexpr UINT lightsCBSlot = static_cast<UINT>(CBSlot::Lights);
        context->PSSetConstantBuffers(lightsCBSlot, 1, m_lightsBuffer.GetAddressOf());

        //----------------------------------------------------------
        // G-Buffer (t9〜t12)、深度 (t13)、ワールド座標 (t14) をバインド
        //----------------------------------------------------------
        ID3D11ShaderResourceView* gbufferSRVs[6] = {
            m_graphicsContext->GetGBufferSRV(0),
            m_graphicsContext->GetGBufferSRV(1),
            m_graphicsContext->GetGBufferSRV(2),
            m_graphicsContext->GetGBufferSRV(3),
            m_graphicsContext->GetDepthSRV(),
            m_graphicsContext->GetGBufferSRV(4),
        };
        constexpr UINT gbufferSRVSlot = static_cast<UINT>(SRVSlot::GBufferAlbedo);
        context->PSSetShaderResources(gbufferSRVSlot, 6, gbufferSRVs);

        //----------------------------------------------------------
        // シャドウマップ (t8/s8) をバインド
        //----------------------------------------------------------
        constexpr UINT shadowSRVSlot = static_cast<UINT>(SRVSlot::ShadowMap);
        m_shadowPass->BindForSampling(context);

        //----------------------------------------------------------
        // G-Bufferサンプラー (s9)：フィルタなしのポイントサンプリング
        //----------------------------------------------------------
        ID3D11SamplerState* pointClamp = m_resources->GetSampler(Tsukino::GraphicsCommon::SamplerType::PointClamp);
        constexpr UINT      gbufferSamplerSlot = static_cast<UINT>(SamplerSlot::GBuffer);
        context->PSSetSamplers(gbufferSamplerSlot, 1, &pointClamp);

        //----------------------------------------------------------
        // IBL (b10, t17〜t19, s10)：スカイ由来のアンビエント
        //----------------------------------------------------------
        m_iblBaker->Bind();

        //----------------------------------------------------------
        // フルスクリーントライアングル描画
        //----------------------------------------------------------
        FullscreenPass::Draw(context);

        //----------------------------------------------------------
        // 後片付け：G-Buffer/深度/シャドウマップ/IBLのSRVを解除
        // （直後にDSVとして再バインドする深度との同時バインド防止のため必須）
        //----------------------------------------------------------
        ID3D11ShaderResourceView* nullSRVs[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        context->PSSetShaderResources(gbufferSRVSlot, 6, nullSRVs);
        ID3D11ShaderResourceView* nullShadowSRV = nullptr;
        context->PSSetShaderResources(shadowSRVSlot, 1, &nullShadowSRV);
        m_iblBaker->Unbind();

        //----------------------------------------------------------
        // 深度ステートを元に戻す（HDRRenderTarget復帰後のWorld/Transparent用）
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_resources->GetCommonStatesTK()->DepthDefault(), 0);
    }
}    // namespace Tsukino::Renderer

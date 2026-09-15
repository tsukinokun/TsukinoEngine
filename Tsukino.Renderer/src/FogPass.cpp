//----------------------------------------------------------------------------
//! @file   FogPass.cpp
//! @brief  フォグパスの実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/FogPass.hpp>

#include "FullscreenPass.hpp"

#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>
#include <Tsukino/Renderer/RenderResources.hpp>
#include <Tsukino/Renderer/FrameConstants.hpp>
#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 定数バッファ（b9）とピクセルシェーダーを作成します。
    //------------------------------------------------------------------------
    bool FogPass::Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants,
                             const FullscreenPass& fullscreenPass, const Tsukino::Asset::ShaderAsset* ps) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;
        m_fullscreenPass  = &fullscreenPass;

        ID3D11Device* device = graphicsContext.GetDevice();

        D3D11_BUFFER_DESC desc = {};
        desc.Usage             = D3D11_USAGE_DEFAULT;
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        desc.ByteWidth         = sizeof(CBufferFog);
        if(FAILED(device->CreateBuffer(&desc, nullptr, m_buffer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create fog constant buffer.");
            m_buffer.Reset();
            return false;
        }

        if(!ps) {
            Tsukino::Core::Log::Error("FogPass::Initialize - shader is null.");
            return false;
        }

        if(FAILED(device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_ps.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create fog pixel shader.");
            m_ps.Reset();
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! フォグを HDR バッファへ合成します（深度(t13)だけを読むので、HDR を RTV に張ったままでよい）。
    //------------------------------------------------------------------------
    void FogPass::Execute() {
        if(!m_enabled || !m_ps || !m_buffer || !m_fullscreenPass->IsValid())
            return;

        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        //----------------------------------------------------------
        // HDRのみをRTVにバインド（深度をSRVとして読むためDSVは外す）
        //----------------------------------------------------------
        m_graphicsContext->BindHDRTargetOnly();

        //----------------------------------------------------------
        // 深度(t13)とポイントサンプラー(s9)をバインド
        //----------------------------------------------------------
        constexpr UINT            depthSRVSlot = static_cast<UINT>(SRVSlot::GBufferDepth);
        ID3D11ShaderResourceView* depthSRV     = m_graphicsContext->GetDepthSRV();
        context->PSSetShaderResources(depthSRVSlot, 1, &depthSRV);

        ID3D11SamplerState* pointClamp         = m_resources->GetSampler(Tsukino::GraphicsCommon::SamplerType::PointClamp);
        constexpr UINT      gbufferSamplerSlot = static_cast<UINT>(SamplerSlot::GBuffer);
        context->PSSetSamplers(gbufferSamplerSlot, 1, &pointClamp);

        //----------------------------------------------------------
        // Scene (b0) をバインド（invViewProj・cameraPos・lightDirを使う）
        //----------------------------------------------------------
        constexpr UINT sceneCBSlot = static_cast<UINT>(CBSlot::Scene);
        context->PSSetConstantBuffers(sceneCBSlot, 1, m_frameConstants->GetSceneBufferAddress());

        //----------------------------------------------------------
        // パラメータ (b9) を更新してバインド
        //----------------------------------------------------------
        context->UpdateSubresource(m_buffer.Get(), 0, nullptr, &m_data, 0, 0);
        constexpr UINT fogCBSlot = static_cast<UINT>(CBSlot::Fog);
        context->PSSetConstantBuffers(fogCBSlot, 1, m_buffer.GetAddressOf());

        //----------------------------------------------------------
        // シェーダーをセット（VSはフルスクリーン三角形用を共用）
        //----------------------------------------------------------
        context->VSSetShader(m_fullscreenPass->GetVertexShader(), nullptr, 0);
        context->PSSetShader(m_ps.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        //----------------------------------------------------------
        // 深度なし・プリマルチプライアルファでover合成
        // （DirectXTKのAlphaBlendは ONE / INV_SRC_ALPHA なので、
        //   PSがfloat4(color * f, f)を返せばそのまま正しいoverになる）
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_resources->GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources->GetCommonStatesTK()->AlphaBlend(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources->GetCommonStatesTK()->CullNone());

        //----------------------------------------------------------
        // フルスクリーントライアングル描画
        //----------------------------------------------------------
        FullscreenPass::Draw(context);

        //----------------------------------------------------------
        // 後片付け：深度を次フレームDSVとして再バインドするため、
        // SRVのバインドを必ず解除する（LightingPass::Executeと同じ理由）。
        // ブレンドも不透明へ戻しておく
        //----------------------------------------------------------
        ID3D11ShaderResourceView* nullSRV = nullptr;
        context->PSSetShaderResources(depthSRVSlot, 1, &nullSRV);

        context->OMSetBlendState(m_resources->GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
    }
}    // namespace Tsukino::Renderer

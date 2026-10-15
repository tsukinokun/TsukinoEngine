//----------------------------------------------------------------------------
//! @file   SphereColorGradePass.cpp
//! @brief  球形の色調補正パスの実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/SphereColorGradePass.hpp>

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
    //! 定数バッファ（b8）・ブレンドステート・ピクセルシェーダーを作成します。
    //------------------------------------------------------------------------
    bool SphereColorGradePass::Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants,
                                          const FullscreenPass& fullscreenPass, const Tsukino::Asset::ShaderAsset* ps) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;
        m_fullscreenPass  = &fullscreenPass;

        ID3D11Device* device = graphicsContext.GetDevice();

        D3D11_BUFFER_DESC desc = {};
        desc.Usage             = D3D11_USAGE_DEFAULT;
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        desc.ByteWidth         = sizeof(CBufferSphereColorGrade);
        if(FAILED(device->CreateBuffer(&desc, nullptr, m_buffer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create sphere color grade constant buffer.");
            m_buffer.Reset();
            return false;
        }

        //----------------------------------------------------------
        // 結果 = 描かれている色 * SV_Target1 + SV_Target0。
        // 乗算（暗くする・色を寄せる）と加算（霧の色・境界の光）を1回で掛けられ、
        // HDR を SRV として読むためのコピーが要らない。アルファはそのまま残す
        //----------------------------------------------------------
        D3D11_BLEND_DESC blendDesc                      = {};
        blendDesc.RenderTarget[0].BlendEnable           = TRUE;
        blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_SRC1_COLOR;
        blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if(FAILED(device->CreateBlendState(&blendDesc, m_blendState.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create sphere color grade blend state.");
            m_blendState.Reset();
            return false;
        }

        if(!ps) {
            Tsukino::Core::Log::Error("SphereColorGradePass::Initialize - shader is null.");
            return false;
        }

        if(FAILED(device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_ps.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create sphere color grade pixel shader.");
            m_ps.Reset();
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! 色調補正を HDR バッファへ掛けます（深度(t13)だけを読むので、HDR を RTV に張ったままでよい）。
    //------------------------------------------------------------------------
    void SphereColorGradePass::Execute() {
        if(!m_enabled || !m_ps || !m_buffer || !m_blendState || !m_fullscreenPass->IsValid())
            return;

        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        //----------------------------------------------------------
        // HDRのみをRTVにバインド（深度をSRVとして読むためDSVは外す）
        //----------------------------------------------------------
        m_graphicsContext->BindHDRTargetOnly();

        constexpr UINT            depthSRVSlot = static_cast<UINT>(SRVSlot::GBufferDepth);
        ID3D11ShaderResourceView* depthSRV     = m_graphicsContext->GetDepthSRV();
        context->PSSetShaderResources(depthSRVSlot, 1, &depthSRV);

        ID3D11SamplerState* pointClamp         = m_resources->GetSampler(Tsukino::GraphicsCommon::SamplerType::PointClamp);
        constexpr UINT      gbufferSamplerSlot = static_cast<UINT>(SamplerSlot::GBuffer);
        context->PSSetSamplers(gbufferSamplerSlot, 1, &pointClamp);

        //----------------------------------------------------------
        // Scene (b0) とパラメータ (b8) をバインド
        //----------------------------------------------------------
        constexpr UINT sceneCBSlot = static_cast<UINT>(CBSlot::Scene);
        context->PSSetConstantBuffers(sceneCBSlot, 1, m_frameConstants->GetSceneBufferAddress());

        context->UpdateSubresource(m_buffer.Get(), 0, nullptr, &m_data, 0, 0);
        constexpr UINT gradeCBSlot = static_cast<UINT>(CBSlot::SphereColorGrade);
        context->PSSetConstantBuffers(gradeCBSlot, 1, m_buffer.GetAddressOf());

        context->VSSetShader(m_fullscreenPass->GetVertexShader(), nullptr, 0);
        context->PSSetShader(m_ps.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        context->OMSetDepthStencilState(m_resources->GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_blendState.Get(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources->GetCommonStatesTK()->CullNone());

        FullscreenPass::Draw(context);

        //----------------------------------------------------------
        // 後片付け（FogPass::Executeと同じ）。深度のSRVと b8 を外し、ブレンドを不透明へ戻す
        //----------------------------------------------------------
        ID3D11ShaderResourceView* nullSRV = nullptr;
        context->PSSetShaderResources(depthSRVSlot, 1, &nullSRV);

        ID3D11Buffer* nullBuffer = nullptr;
        context->PSSetConstantBuffers(gradeCBSlot, 1, &nullBuffer);

        context->OMSetBlendState(m_resources->GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
    }
}    // namespace Tsukino::Renderer

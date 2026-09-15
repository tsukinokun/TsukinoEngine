//----------------------------------------------------------------------------
//! @file   SkyPass.cpp
//! @brief  スカイ（大気散乱）パスの実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/SkyPass.hpp>

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
    //! スカイの定数バッファ（b4）を作成します。
    //------------------------------------------------------------------------
    bool SkyPass::Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;

        D3D11_BUFFER_DESC desc = {};
        desc.Usage             = D3D11_USAGE_DEFAULT;
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        desc.ByteWidth         = sizeof(CBufferSky);

        if(FAILED(graphicsContext.GetDevice()->CreateBuffer(&desc, nullptr, m_buffer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create sky constant buffer.");
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! スカイのシェーダーを作成して、パスを有効にします。
    //------------------------------------------------------------------------
    void SkyPass::SetPipeline(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps) {
        if(!vs || !ps) {
            Tsukino::Core::Log::Error("SkyPass::SetPipeline - shader is null.");
            return;
        }

        ID3D11Device* device = m_graphicsContext->GetDevice();

        if(FAILED(device->CreateVertexShader(vs->binary.data(), vs->binary.size(), nullptr, m_vs.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create sky vertex shader.");
            return;
        }

        if(FAILED(device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_ps.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create sky pixel shader.");
            return;
        }

        m_hasPipeline = true;
    }

    //------------------------------------------------------------------------
    //! スカイを描きます（パラメータを b4 へ転送してから描きます）。
    //------------------------------------------------------------------------
    void SkyPass::Execute() {
        if(!m_hasPipeline || !m_vs || !m_ps)
            return;

        ID3D11DeviceContext*   context = m_graphicsContext->GetContext();
        DirectX::CommonStates* states  = m_resources->GetCommonStatesTK();

        // シェーダーをセット（頂点バッファ不要）
        context->VSSetShader(m_vs.Get(), nullptr, 0);
        context->PSSetShader(m_ps.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        // 深度書き込みなし（スカイは常に最背面）
        context->OMSetDepthStencilState(states->DepthRead(), 0);
        context->OMSetBlendState(states->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(states->CullNone());

        // Scene (b0) をバインド（invViewProjの計算に使う）
        context->VSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_frameConstants->GetSceneBufferAddress());
        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Scene), 1, m_frameConstants->GetSceneBufferAddress());

        // Sky (b4) をバインド
        context->UpdateSubresource(m_buffer.Get(), 0, nullptr, &m_data, 0, 0);
        context->PSSetConstantBuffers(static_cast<UINT>(CBSlot::Sky), 1, m_buffer.GetAddressOf());

        // 頂点バッファなしでフルスクリーントライアングルを描画（VSでSV_VertexIDから3頂点を生成する）
        FullscreenPass::Draw(context);

        // ステートをリセット
        context->OMSetDepthStencilState(states->DepthDefault(), 0);
    }
}    // namespace Tsukino::Renderer

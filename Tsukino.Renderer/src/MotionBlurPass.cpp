//----------------------------------------------------------------------------
//! @file   MotionBlurPass.cpp
//! @brief  モーションブラーパスの実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/MotionBlurPass.hpp>

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
    //! 定数バッファ（b8）とピクセルシェーダーを作成します。
    //------------------------------------------------------------------------
    bool MotionBlurPass::Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants,
                                    const FullscreenPass& fullscreenPass, const Tsukino::Asset::ShaderAsset* ps) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;
        m_fullscreenPass  = &fullscreenPass;

        ID3D11Device* device = graphicsContext.GetDevice();

        D3D11_BUFFER_DESC desc = {};
        desc.Usage             = D3D11_USAGE_DEFAULT;
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        desc.ByteWidth         = sizeof(CBufferMotionBlur);
        if(FAILED(device->CreateBuffer(&desc, nullptr, m_buffer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create motion blur constant buffer.");
            m_buffer.Reset();
            return false;
        }

        if(!ps) {
            Tsukino::Core::Log::Error("MotionBlurPass::Initialize - shader is null.");
            return false;
        }

        if(FAILED(device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_ps.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create motion blur pixel shader.");
            m_ps.Reset();
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! モーションブラーを掛けてポストプロセス用の中間バッファへ書きます（HDR(t0) と速度バッファ(t15) を読みます）。
    //------------------------------------------------------------------------
    bool MotionBlurPass::Execute() {
        if(!m_enabled || !m_ps || !m_buffer || !m_fullscreenPass->IsValid())
            return false;

        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        //----------------------------------------------------------
        // ポストプロセス用中間バッファへ切り替え
        // （HDRをSRVとして読むため、HDRをRTVに残したままにはできない）
        //----------------------------------------------------------
        m_graphicsContext->BindPostProcessTarget();

        //----------------------------------------------------------
        // シーンカラー(t0/s0)と速度バッファ(t15/s9)をバインド
        //----------------------------------------------------------
        ID3D11ShaderResourceView* hdrSRV = m_graphicsContext->GetHDRSRV();
        context->PSSetShaderResources(0, 1, &hdrSRV);

        ID3D11SamplerState* linearClamp = m_resources->GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp);
        context->PSSetSamplers(0, 1, &linearClamp);

        constexpr UINT            velocitySRVSlot = static_cast<UINT>(SRVSlot::GBufferVelocity);
        ID3D11ShaderResourceView* velocitySRV     = m_graphicsContext->GetGBufferSRV(5);
        context->PSSetShaderResources(velocitySRVSlot, 1, &velocitySRV);

        ID3D11SamplerState* pointClamp = m_resources->GetSampler(Tsukino::GraphicsCommon::SamplerType::PointClamp);
        constexpr UINT      gbufferSamplerSlot = static_cast<UINT>(SamplerSlot::GBuffer);
        context->PSSetSamplers(gbufferSamplerSlot, 1, &pointClamp);

        //----------------------------------------------------------
        // パラメータ (b8) を更新してバインド
        //----------------------------------------------------------
        context->UpdateSubresource(m_buffer.Get(), 0, nullptr, &m_data, 0, 0);
        constexpr UINT motionBlurCBSlot = static_cast<UINT>(CBSlot::MotionBlur);
        context->PSSetConstantBuffers(motionBlurCBSlot, 1, m_buffer.GetAddressOf());

        //----------------------------------------------------------
        // シェーダーをセット（VSはフルスクリーン三角形用を共用）
        //----------------------------------------------------------
        context->VSSetShader(m_fullscreenPass->GetVertexShader(), nullptr, 0);
        context->PSSetShader(m_ps.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        //----------------------------------------------------------
        // 深度なし・ブレンドなし
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_resources->GetCommonStatesTK()->DepthNone(), 0);
        context->OMSetBlendState(m_resources->GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources->GetCommonStatesTK()->CullNone());

        //----------------------------------------------------------
        // フルスクリーントライアングル描画
        //----------------------------------------------------------
        FullscreenPass::Draw(context);

        //----------------------------------------------------------
        // 後片付け：次フレームでHDR/G-BufferをRTVとして再バインドするため、
        // SRVのバインドを必ず解除する（LightingPass::Executeと同じ理由）
        //----------------------------------------------------------
        ID3D11ShaderResourceView* nullSRV = nullptr;
        context->PSSetShaderResources(0, 1, &nullSRV);
        context->PSSetShaderResources(velocitySRVSlot, 1, &nullSRV);

        return true;
    }
}    // namespace Tsukino::Renderer

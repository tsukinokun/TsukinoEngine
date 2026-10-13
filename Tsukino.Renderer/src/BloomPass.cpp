//----------------------------------------------------------------------------
//! @file   BloomPass.cpp
//! @brief  ブルームパスの実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/BloomPass.hpp>

#include "FullscreenPass.hpp"

#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>
#include <Tsukino/Renderer/RenderResources.hpp>
#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>

#include <string>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    namespace {
        //! これより小さい縮小段は作らない（2px を切ると 13 タップが同じテクセルを読むだけになる）
        constexpr UINT kMinMipSize = 2;

        //--------------------------------------------------------------------
        //! シェーダーアセットからピクセルシェーダーを作ります。
        //--------------------------------------------------------------------
        bool CreatePS(ID3D11Device* device, const Tsukino::Asset::ShaderAsset* asset, Microsoft::WRL::ComPtr<ID3D11PixelShader>& out,
                      const char* name) {
            if(!asset) {
                Tsukino::Core::Log::Error(std::string("BloomPass::Initialize - shader is null: ") + name);
                return false;
            }
            if(FAILED(device->CreatePixelShader(asset->binary.data(), asset->binary.size(), nullptr, out.GetAddressOf()))) {
                Tsukino::Core::Log::Error(std::string("Failed to create bloom pixel shader: ") + name);
                out.Reset();
                return false;
            }
            return true;
        }
    }    // namespace

    //------------------------------------------------------------------------
    //! 定数バッファとピクセルシェーダーを作成します。
    //------------------------------------------------------------------------
    bool BloomPass::Initialize(GraphicsContext& graphicsContext, RenderResources& resources, const FullscreenPass& fullscreenPass,
                               const Tsukino::Asset::ShaderAsset* prefilterPS, const Tsukino::Asset::ShaderAsset* downsamplePS,
                               const Tsukino::Asset::ShaderAsset* upsamplePS) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_fullscreenPass  = &fullscreenPass;

        ID3D11Device* device = graphicsContext.GetDevice();

        D3D11_BUFFER_DESC desc = {};
        desc.Usage             = D3D11_USAGE_DEFAULT;
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        desc.ByteWidth         = sizeof(CBufferBloom);
        if(FAILED(device->CreateBuffer(&desc, nullptr, m_buffer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create bloom constant buffer.");
            m_buffer.Reset();
            return false;
        }

        bool ok = CreatePS(device, prefilterPS, m_prefilterPS, "prefilter");
        ok      = CreatePS(device, downsamplePS, m_downsamplePS, "downsample") && ok;
        ok      = CreatePS(device, upsamplePS, m_upsamplePS, "upsample") && ok;
        return ok;
    }

    //------------------------------------------------------------------------
    //! 縮小段のテクスチャを画面サイズに合わせて作り直します。
    //------------------------------------------------------------------------
    bool BloomPass::EnsureMipTargets(UINT width, UINT height) {
        if(width == m_sourceWidth && height == m_sourceHeight && m_mipCount > 0)
            return true;

        for(MipTarget& mip : m_mips)
            mip = MipTarget{};
        m_mipCount     = 0;
        m_sourceWidth  = width;
        m_sourceHeight = height;

        ID3D11Device* device = m_graphicsContext->GetDevice();

        UINT mipWidth  = width / 2;
        UINT mipHeight = height / 2;
        for(u32 i = 0; i < kMaxMipCount; ++i) {
            if(mipWidth < kMinMipSize || mipHeight < kMinMipSize)
                break;

            //--------------------------------------------------------------
            // HDR と同じ 16bit 浮動小数。閾値を超えた明るさ（1.0 超）を
            // そのまま持ち運ぶので、8bit では足りない
            //--------------------------------------------------------------
            D3D11_TEXTURE2D_DESC texDesc{};
            texDesc.Width            = mipWidth;
            texDesc.Height           = mipHeight;
            texDesc.MipLevels        = 1;
            texDesc.ArraySize        = 1;
            texDesc.Format           = DXGI_FORMAT_R16G16B16A16_FLOAT;
            texDesc.SampleDesc.Count = 1;
            texDesc.Usage            = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags        = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

            MipTarget& mip = m_mips[i];
            if(FAILED(device->CreateTexture2D(&texDesc, nullptr, mip.texture.GetAddressOf())) ||
               FAILED(device->CreateRenderTargetView(mip.texture.Get(), nullptr, mip.rtv.GetAddressOf())) ||
               FAILED(device->CreateShaderResourceView(mip.texture.Get(), nullptr, mip.srv.GetAddressOf()))) {
                Tsukino::Core::Log::Error("BloomPass: Failed to create bloom mip " + std::to_string(i) + ".");
                mip = MipTarget{};
                break;
            }
            mip.width  = mipWidth;
            mip.height = mipHeight;
            ++m_mipCount;

            mipWidth /= 2;
            mipHeight /= 2;
        }

        return m_mipCount > 0;
    }

    //------------------------------------------------------------------------
    //! 定数バッファを書き換えてフルスクリーン三角形を1枚描きます。
    //------------------------------------------------------------------------
    void BloomPass::DrawFullscreen(ID3D11ShaderResourceView* source, UINT sourceWidth, UINT sourceHeight, UINT targetWidth,
                                   UINT targetHeight, CBufferBloom data) {
        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        data.texelSizeX = 1.0f / static_cast<float>(sourceWidth);
        data.texelSizeY = 1.0f / static_cast<float>(sourceHeight);
        context->UpdateSubresource(m_buffer.Get(), 0, nullptr, &data, 0, 0);

        D3D11_VIEWPORT vp{};
        vp.Width    = static_cast<float>(targetWidth);
        vp.Height   = static_cast<float>(targetHeight);
        vp.MaxDepth = 1.0f;
        context->RSSetViewports(1, &vp);

        context->PSSetShaderResources(0, 1, &source);
        FullscreenPass::Draw(context);

        // 次の描画でこのテクスチャを RTV に張るので、SRV のバインドを毎回外す
        ID3D11ShaderResourceView* nullSRV = nullptr;
        context->PSSetShaderResources(0, 1, &nullSRV);
    }

    //------------------------------------------------------------------------
    //! ブルームを掛けて、トーンマップの入力になるバッファへ加算します。
    //------------------------------------------------------------------------
    void BloomPass::Execute(bool sourceIsPostProcess) {
        if(!m_enabled || !m_buffer || !m_prefilterPS || !m_downsamplePS || !m_upsamplePS || !m_fullscreenPass->IsValid())
            return;

        const UINT width  = m_graphicsContext->GetWidth();
        const UINT height = m_graphicsContext->GetHeight();
        if(!EnsureMipTargets(width, height))
            return;

        ID3D11DeviceContext*    context = m_graphicsContext->GetContext();
        DirectX::CommonStates* states  = m_resources->GetCommonStatesTK();

        ID3D11ShaderResourceView* sceneSRV = sourceIsPostProcess ? m_graphicsContext->GetPostProcessSRV() : m_graphicsContext->GetHDRSRV();

        //----------------------------------------------------------
        // 共通のステート（VS はフルスクリーン三角形用を共用）
        //----------------------------------------------------------
        context->VSSetShader(m_fullscreenPass->GetVertexShader(), nullptr, 0);
        context->IASetInputLayout(nullptr);
        context->OMSetDepthStencilState(states->DepthNone(), 0);
        context->RSSetState(states->CullNone());

        ID3D11SamplerState* linearClamp = m_resources->GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp);
        context->PSSetSamplers(0, 1, &linearClamp);

        constexpr UINT bloomCBSlot = static_cast<UINT>(CBSlot::Bloom);
        context->PSSetConstantBuffers(bloomCBSlot, 1, m_buffer.GetAddressOf());

        CBufferBloom data{};
        data.threshold = m_params.threshold;
        data.softKnee  = m_params.softKnee;
        data.radius    = m_params.radius;

        //----------------------------------------------------------
        // 1) 明部抽出 + 1/2 へ縮小（シーン → mip0）
        //----------------------------------------------------------
        context->OMSetBlendState(states->Opaque(), nullptr, 0xFFFFFFFF);
        context->PSSetShader(m_prefilterPS.Get(), nullptr, 0);
        context->OMSetRenderTargets(1, m_mips[0].rtv.GetAddressOf(), nullptr);
        DrawFullscreen(sceneSRV, width, height, m_mips[0].width, m_mips[0].height, data);

        //----------------------------------------------------------
        // 2) 縮小（mip[i-1] → mip[i]）
        //----------------------------------------------------------
        context->PSSetShader(m_downsamplePS.Get(), nullptr, 0);
        for(u32 i = 1; i < m_mipCount; ++i) {
            context->OMSetRenderTargets(1, m_mips[i].rtv.GetAddressOf(), nullptr);
            DrawFullscreen(m_mips[i - 1].srv.Get(), m_mips[i - 1].width, m_mips[i - 1].height, m_mips[i].width, m_mips[i].height, data);
        }

        //----------------------------------------------------------
        // 3) 拡大しながら足し戻す（mip[i] → mip[i-1] へ加算）
        //    小さい段ほど広く滲んでいるので、全段を足すと「芯は強く、裾は広い」光になる
        //----------------------------------------------------------
        context->OMSetBlendState(states->Additive(), nullptr, 0xFFFFFFFF);
        context->PSSetShader(m_upsamplePS.Get(), nullptr, 0);
        for(u32 i = m_mipCount - 1; i > 0; --i) {
            context->OMSetRenderTargets(1, m_mips[i - 1].rtv.GetAddressOf(), nullptr);
            DrawFullscreen(m_mips[i].srv.Get(), m_mips[i].width, m_mips[i].height, m_mips[i - 1].width, m_mips[i - 1].height, data);
        }

        //----------------------------------------------------------
        // 4) 元の絵へ加算（mip0 → シーン）
        //    書き込み先は読み出し元と同じバッファ。ここでは mip0 しか読まないので
        //    RTV と SRV の同時バインドにはならない
        //----------------------------------------------------------
        data.intensity = m_params.intensity;
        data.tintR     = m_params.tintR;
        data.tintG     = m_params.tintG;
        data.tintB     = m_params.tintB;
        if(sourceIsPostProcess)
            m_graphicsContext->BindPostProcessTarget();
        else
            m_graphicsContext->BindHDRTargetOnly();
        DrawFullscreen(m_mips[0].srv.Get(), m_mips[0].width, m_mips[0].height, width, height, data);

        //----------------------------------------------------------
        // 後片付け：ビューポートを画面全体へ戻し、ブレンドを不透明へ戻す
        //----------------------------------------------------------
        D3D11_VIEWPORT vp{};
        vp.Width    = static_cast<float>(width);
        vp.Height   = static_cast<float>(height);
        vp.MaxDepth = 1.0f;
        context->RSSetViewports(1, &vp);
        context->OMSetBlendState(states->Opaque(), nullptr, 0xFFFFFFFF);
    }
}    // namespace Tsukino::Renderer

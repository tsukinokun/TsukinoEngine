//----------------------------------------------------------------------------
//! @file   TonemapPass.cpp
//! @brief  トーンマップパスの実装
//----------------------------------------------------------------------------
#include "TonemapPass.hpp"
#include "FullscreenPass.hpp"

#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>
#include <Tsukino/Renderer/RenderResources.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! ピクセルシェーダーを作成します。
    //------------------------------------------------------------------------
    bool TonemapPass::Initialize(GraphicsContext& graphicsContext, RenderResources& resources, const FullscreenPass& fullscreenPass,
                                 const Tsukino::Asset::ShaderAsset* ps) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_fullscreenPass  = &fullscreenPass;

        if(!ps) {
            Tsukino::Core::Log::Error("TonemapPass::Initialize - shader is null.");
            return false;
        }

        if(FAILED(graphicsContext.GetDevice()->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_ps.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create tonemap pixel shader.");
            m_ps.Reset();
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! トーンマップを実行してバックバッファへ書きます。
    //------------------------------------------------------------------------
    void TonemapPass::Execute(ID3D11ShaderResourceView* source) {
        if(!m_ps || !m_fullscreenPass->IsValid())
            return;

        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        // バックバッファに切り替え（入力SRVとRTVの同時バインド防止）
        m_graphicsContext->BindBackBuffer();

        //--------------------------------------------------------------------
        // 入力となるシーンカラーをt0にバインド
        // （モーションブラーが走った場合はポストプロセス用中間バッファ、
        //   走らなかった場合はHDRバッファがそのまま渡ってくる）
        //--------------------------------------------------------------------
        ID3D11ShaderResourceView* hdrSRV = source;
        context->PSSetShaderResources(0, 1, &hdrSRV);

        // LinearClampサンプラーをs0にバインド
        ID3D11SamplerState* sampler = m_resources->GetSampler(Tsukino::GraphicsCommon::SamplerType::LinearClamp);
        context->PSSetSamplers(0, 1, &sampler);

        // シェーダーをセット
        context->VSSetShader(m_fullscreenPass->GetVertexShader(), nullptr, 0);
        context->PSSetShader(m_ps.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        // 深度なし・ブレンドなし
        DirectX::CommonStates* states = m_resources->GetCommonStatesTK();
        context->OMSetDepthStencilState(states->DepthNone(), 0);
        context->OMSetBlendState(states->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(states->CullNone());

        FullscreenPass::Draw(context);

        // HDR SRVのバインドを解除
        ID3D11ShaderResourceView* nullSRV = nullptr;
        context->PSSetShaderResources(0, 1, &nullSRV);
    }
}    // namespace Tsukino::Renderer

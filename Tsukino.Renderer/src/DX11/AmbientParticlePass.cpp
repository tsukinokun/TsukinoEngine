//----------------------------------------------------------------------------
//! @file   AmbientParticlePass.cpp
//! @brief  環境パーティクル（火の粉・灰）パスの実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/AmbientParticlePass.hpp>
#include <Tsukino/Renderer/DX11/FrameConstantsGpu.hpp>
#include <Tsukino/Renderer/DX11/AmbientParticlePassGpu.hpp>
#include <Tsukino/Renderer/DX11/RenderResourcesGpu.hpp>

#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>
#include <Tsukino/Renderer/RenderResources.hpp>
#include <Tsukino/Renderer/FrameConstants.hpp>
#include <Tsukino/Renderer/ShaderSlots.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 定数バッファ（b10）とシェーダーを作成します。
    //------------------------------------------------------------------------
    bool AmbientParticlePass::Initialize(GraphicsContext& graphicsContext, RenderResources& resources, FrameConstants& frameConstants,
                                         const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;

        ID3D11Device* device = graphicsContext.GetDevice();

        D3D11_BUFFER_DESC desc = {};
        desc.Usage             = D3D11_USAGE_DEFAULT;
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
        desc.ByteWidth         = sizeof(CBufferAmbientParticle);
        if(FAILED(device->CreateBuffer(&desc, nullptr, m_gpu->buffer.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create ambient particle constant buffer.");
            m_gpu->buffer.Reset();
            return false;
        }

        if(!vs || !ps) {
            Tsukino::Core::Log::Error("AmbientParticlePass::Initialize - shader is null.");
            return false;
        }

        if(FAILED(device->CreateVertexShader(vs->binary.data(), vs->binary.size(), nullptr, m_gpu->vs.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create ambient particle vertex shader.");
            m_gpu->vs.Reset();
            return false;
        }

        if(FAILED(device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_gpu->ps.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create ambient particle pixel shader.");
            m_gpu->ps.Reset();
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! 環境パーティクルを HDR バッファへ加算合成で描きます。
    //------------------------------------------------------------------------
    void AmbientParticlePass::Execute() {
        if(!m_enabled || !m_gpu->vs || !m_gpu->ps || !m_gpu->buffer || m_count == 0)
            return;

        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

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
        context->VSSetConstantBuffers(sceneCBSlot, 1, m_frameConstants->GetGpu().GetSceneBufferAddress());

        //----------------------------------------------------------
        // パラメータ (b10) を更新してバインド（頂点シェーダー専用）
        //----------------------------------------------------------
        context->UpdateSubresource(m_gpu->buffer.Get(), 0, nullptr, &m_data, 0, 0);
        constexpr UINT particleCBSlot = static_cast<UINT>(CBSlot::AmbientParticle);
        context->VSSetConstantBuffers(particleCBSlot, 1, m_gpu->buffer.GetAddressOf());

        //----------------------------------------------------------
        // シェーダーをセット（頂点バッファが無いので入力レイアウトも不要）
        //----------------------------------------------------------
        context->VSSetShader(m_gpu->vs.Get(), nullptr, 0);
        context->PSSetShader(m_gpu->ps.Get(), nullptr, 0);
        context->IASetInputLayout(nullptr);

        //----------------------------------------------------------
        // 深度テストあり・深度書き込みなし・加算合成
        //
        // DepthReadReverseZはGREATER_EQUALなのでこのエンジンのリバースZと
        // 一致する（DepthReadはLESS_EQUALなので使ってはいけない）。
        // 深度を書かないことで粒子どうしの前後関係を気にせずに済み、
        // 加算合成なので描画順のソートも不要になる
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_resources->GetGpu().GetCommonStatesTK()->DepthReadReverseZ(), 0);
        context->OMSetBlendState(m_resources->GetGpu().GetCommonStatesTK()->Additive(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_resources->GetGpu().GetCommonStatesTK()->CullNone());

        //----------------------------------------------------------
        // 頂点バッファなしで「1粒 = 三角形2枚」を一括描画
        //----------------------------------------------------------
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->Draw(m_count * 6, 0);

        //----------------------------------------------------------
        // ステートを戻す（Sky / Fogと同じ流儀）
        //----------------------------------------------------------
        context->OMSetDepthStencilState(m_resources->GetGpu().GetCommonStatesTK()->DepthDefault(), 0);
        context->OMSetBlendState(m_resources->GetGpu().GetCommonStatesTK()->Opaque(), nullptr, 0xFFFFFFFF);
    }

    //------------------------------------------------------------------------
    //! 資源を持たない状態で作ります。
    //------------------------------------------------------------------------
    AmbientParticlePass::AmbientParticlePass()
        : m_gpu(std::make_unique<Gpu>()) {}

    //------------------------------------------------------------------------
    //! 資源を破棄します。
    //------------------------------------------------------------------------
    AmbientParticlePass::~AmbientParticlePass() = default;
}    // namespace Tsukino::Renderer

//----------------------------------------------------------------------------
//! @file   DebugDraw.cpp
//! @brief  デバッグ用の線と三角形の描画の実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/DebugDraw.hpp>

#include <Tsukino/Renderer/DX11/GraphicsContext.hpp>
#include <Tsukino/Renderer/RenderResources.hpp>
#include <Tsukino/Renderer/FrameConstants.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! シェーダーと頂点バッファを作成します。
    //------------------------------------------------------------------------
    bool DebugDraw::Initialize(GraphicsContext&                   graphicsContext,
                               RenderResources&                   resources,
                               FrameConstants&                    frameConstants,
                               const Tsukino::Asset::ShaderAsset* vs,
                               const Tsukino::Asset::ShaderAsset* ps) {
        m_graphicsContext = &graphicsContext;
        m_resources       = &resources;
        m_frameConstants  = &frameConstants;

        if(!vs || !ps) {
            Tsukino::Core::Log::Error("Debug shader assets are null.");
            return false;
        }

        ID3D11Device* device = graphicsContext.GetDevice();

        // 頂点シェーダーの作成
        if(FAILED(device->CreateVertexShader(vs->binary.data(), vs->binary.size(), nullptr, m_vs.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create debug vertex shader.");
            return false;
        }

        // ピクセルシェーダーの作成
        if(FAILED(device->CreatePixelShader(ps->binary.data(), ps->binary.size(), nullptr, m_ps.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create debug pixel shader.");
            return false;
        }

        // 入力レイアウトの作成
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Tsukino::GraphicsCommon::DebugVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Tsukino::GraphicsCommon::DebugVertex, color),    D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        if(FAILED(device->CreateInputLayout(layout, ARRAYSIZE(layout), vs->binary.data(), vs->binary.size(), m_inputLayout.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create debug input layout.");
            return false;
        }

        // 動的頂点バッファの作成
        D3D11_BUFFER_DESC bd{};
        bd.Usage          = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth      = static_cast<UINT>(sizeof(Tsukino::GraphicsCommon::DebugVertex) * kMaxVertices);
        bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        if(FAILED(device->CreateBuffer(&bd, nullptr, m_lineVB.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create debug line vertex buffer.");
            return false;
        }

        if(FAILED(device->CreateBuffer(&bd, nullptr, m_triangleVB.GetAddressOf()))) {
            Tsukino::Core::Log::Error("Failed to create debug triangle vertex buffer.");
            return false;
        }

        return true;
    }

    //------------------------------------------------------------------------
    //! 線を1本積みます。
    //------------------------------------------------------------------------
    void DebugDraw::DrawLine(const Tsukino::GraphicsCommon::DebugVertex& v1, const Tsukino::GraphicsCommon::DebugVertex& v2) {
        m_lineVertices.push_back(v1);
        m_lineVertices.push_back(v2);
    }

    //------------------------------------------------------------------------
    //! 三角形を1枚積みます（ワイヤーフレームで描かれます）。
    //------------------------------------------------------------------------
    void DebugDraw::DrawTriangle(const Tsukino::GraphicsCommon::DebugVertex& v1,
                                 const Tsukino::GraphicsCommon::DebugVertex& v2,
                                 const Tsukino::GraphicsCommon::DebugVertex& v3) {
        m_triangleVertices.push_back(v1);
        m_triangleVertices.push_back(v2);
        m_triangleVertices.push_back(v3);
    }

    //------------------------------------------------------------------------
    //! 積んだ線と三角形を描画せずに捨てます。
    //------------------------------------------------------------------------
    void DebugDraw::Clear() {
        m_lineVertices.clear();
        m_triangleVertices.clear();
    }

    //------------------------------------------------------------------------
    //! 積んだ線と三角形を描画し、空にします。
    //------------------------------------------------------------------------
    void DebugDraw::Flush() {
        ID3D11DeviceContext* context = m_graphicsContext->GetContext();

        m_frameConstants->UploadWorld();

        if(m_lineVertices.empty() && m_triangleVertices.empty())
            return;

        DirectX::CommonStates* states = m_resources->GetCommonStatesTK();

        context->VSSetShader(m_vs.Get(), nullptr, 0);
        context->PSSetShader(m_ps.Get(), nullptr, 0);
        context->IASetInputLayout(m_inputLayout.Get());

        // 不透明・深度テストあり（リバースZ）・カリングなし
        context->OMSetBlendState(states->Opaque(), nullptr, 0xFFFFFFFF);
        context->OMSetDepthStencilState(states->DepthReverseZ(), 0);
        context->RSSetState(states->CullNone());

        UINT stride = sizeof(Tsukino::GraphicsCommon::DebugVertex);
        UINT offset = 0;

        // --- ラインの描画 ---
        if(!m_lineVertices.empty()) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            if(SUCCEEDED(context->Map(m_lineVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                size_t count = std::min(m_lineVertices.size(), kMaxVertices);
                std::memcpy(mapped.pData, m_lineVertices.data(), count * stride);
                context->Unmap(m_lineVB.Get(), 0);

                context->IASetVertexBuffers(0, 1, m_lineVB.GetAddressOf(), &stride, &offset);
                context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
                context->Draw(static_cast<UINT>(count), 0);
            }
            m_lineVertices.clear();
        }

        // --- 三角形の描画 ---
        if(!m_triangleVertices.empty()) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            if(SUCCEEDED(context->Map(m_triangleVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                size_t count = std::min(m_triangleVertices.size(), kMaxVertices);
                std::memcpy(mapped.pData, m_triangleVertices.data(), count * stride);
                context->Unmap(m_triangleVB.Get(), 0);

                context->IASetVertexBuffers(0, 1, m_triangleVB.GetAddressOf(), &stride, &offset);
                context->RSSetState(states->Wireframe());    // 三角形はワイヤーフレームで描画
                context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                context->Draw(static_cast<UINT>(count), 0);
                context->RSSetState(states->CullNone());    // 元に戻す
            }
            m_triangleVertices.clear();
        }
    }
}    // namespace Tsukino::Renderer

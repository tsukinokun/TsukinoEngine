//------------------------------------------------------------
//! @file   SpriteRenderer.cpp
//! @brief  スプライト描画クラス
//! @author 山﨑愛
//------------------------------------------------------------
#include <Tsukino/Renderer/SpriteRenderer.hpp>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------
    //! @brief 初期化
    //------------------------------------------------------------
    bool SpriteRenderer::Initialize(ID3D11Device* device) {
        //------------------------------------------------------------
        // Quad頂点
        //------------------------------------------------------------
        Vertex vertices[] = {
            {{-0.5f, 0.5f, 0.0f},  {0.0f, 0.0f}},
            {{0.5f, 0.5f, 0.0f},   {1.0f, 0.0f}},
            {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}},
            {{0.5f, -0.5f, 0.0f},  {1.0f, 1.0f}}
        };

        //------------------------------------------------------------
        // Index
        //------------------------------------------------------------
        u32 indices[] = {0, 1, 2, 2, 1, 3};

        //------------------------------------------------------------
        // VertexBuffer
        //------------------------------------------------------------
        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.ByteWidth         = sizeof(vertices);
        vbDesc.Usage             = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData = {};
        vbData.pSysMem                = vertices;

        device->CreateBuffer(&vbDesc, &vbData, m_vertexBuffer.GetAddressOf());

        //------------------------------------------------------------
        // IndexBuffer
        //------------------------------------------------------------
        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.ByteWidth         = sizeof(indices);
        ibDesc.Usage             = D3D11_USAGE_DEFAULT;
        ibDesc.BindFlags         = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA ibData = {};
        ibData.pSysMem                = indices;

        device->CreateBuffer(&ibDesc, &ibData, m_indexBuffer.GetAddressOf());

        //------------------------------------------------------------
        // Sampler
        //------------------------------------------------------------
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter             = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;

        device->CreateSamplerState(&samplerDesc, m_sampler.GetAddressOf());

        return true;
    }

    //------------------------------------------------------------
    //! @brief スプライト描画
    //------------------------------------------------------------
    void SpriteRenderer::Draw(ID3D11DeviceContext* context, DX11Texture2D* texture) {
        //------------------------------------------------------------
        // VertexBuffer
        //------------------------------------------------------------
        UINT stride = sizeof(Vertex);
        UINT offset = 0;

        ID3D11Buffer* vb = m_vertexBuffer.Get();

        context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

        //------------------------------------------------------------
        // IndexBuffer
        //------------------------------------------------------------
        context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

        //------------------------------------------------------------
        // Primitive
        //------------------------------------------------------------
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        //------------------------------------------------------------
        // Sampler
        //------------------------------------------------------------
        context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

        //------------------------------------------------------------
        // Texture Bind
        //------------------------------------------------------------
        texture->Bind(context, 0);

        //------------------------------------------------------------
        // Draw
        //------------------------------------------------------------
        context->DrawIndexed(6, 0, 0);
    }

}    // namespace Tsukino::Renderer

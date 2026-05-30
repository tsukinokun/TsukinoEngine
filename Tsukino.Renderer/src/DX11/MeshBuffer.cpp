//--------------------------------------------------------------
//! @file   MeshBuffer.cpp
//! @brief  メッシュバッファ生成の実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Renderer/DX11/MeshBuffer.hpp>
#include <Tsukino/GraphicsCommon/Mesh/MeshData.hpp>

namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @brief  メッシュデータを作成する関数
    //--------------------------------------------------------------
    MeshBuffer CreateMeshBuffer(ID3D11Device* device, const Tsukino::GraphicsCommon::MeshData& meshData) {
        MeshBuffer buffer{};

        // 頂点データが空なら何も作らない
        if(meshData.vertexData.empty() || meshData.vertexCount == 0) {
            return buffer;
        }

        //--------------------------------------------------------------
        // 頂点バッファの作成
        //--------------------------------------------------------------
        D3D11_BUFFER_DESC vbd = {};
        vbd.Usage             = D3D11_USAGE_DEFAULT;
        vbd.ByteWidth         = static_cast<UINT>(meshData.vertexData.size());
        vbd.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vinit = {};
        vinit.pSysMem                = meshData.vertexData.data();

        HRESULT hr = device->CreateBuffer(&vbd, &vinit, buffer.vertexBuffer.GetAddressOf());

        if(FAILED(hr)) {
            return buffer;
        }

        //--------------------------------------------------------------
        // インデックスバッファの作成
        //--------------------------------------------------------------
        if(!meshData.indices.empty()) {
            D3D11_BUFFER_DESC ibd = {};
            ibd.Usage             = D3D11_USAGE_DEFAULT;
            ibd.ByteWidth         = static_cast<UINT>(meshData.indices.size() * sizeof(u32));
            ibd.BindFlags         = D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA iinit = {};
            iinit.pSysMem                = meshData.indices.data();

            hr = device->CreateBuffer(&ibd, &iinit, buffer.indexBuffer.GetAddressOf());

            if(FAILED(hr)) {
                return buffer;
            }
        }

        if(!meshData.boneWeights.empty()) {    // meshDataにボーンデータが含まれているかチェック
            D3D11_BUFFER_DESC bbd = {};
            bbd.Usage             = D3D11_USAGE_DEFAULT;
            bbd.ByteWidth         = static_cast<UINT>(meshData.boneWeights.size() * sizeof(Tsukino::GraphicsCommon::BoneWeight));
            bbd.BindFlags         = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA binit = {};
            binit.pSysMem                = meshData.boneWeights.data();

            HRESULT hr = device->CreateBuffer(&bbd, &binit, buffer.boneWeightBuffer.GetAddressOf());
            if(FAILED(hr)) {
                // エラーハンドリング
            }
        }

        //--------------------------------------------------------------
        // メタ情報を保存
        //--------------------------------------------------------------
        buffer.vertexCount = meshData.vertexCount;     
        buffer.indexCount  = meshData.indexCount;      
        buffer.stride      = meshData.vertexStride;    

        return buffer;
    }

}    // namespace Tsukino::Renderer

//--------------------------------------------------------------
//! @file   MeshBuffer.hpp
//! @brief  メッシュバッファ構造体の宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

#include <d3d11.h>
#include <wrl/client.h>

// 名前空間 : Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    struct MeshData;    // 前方宣言
}    //namespace Tsukino::GraphicsCommon

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @struct MeshBuffer
    //! @brief  メッシュバッファ構造体
    //--------------------------------------------------------------
    struct MeshBuffer {
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;       // 頂点バッファ
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;        // インデックスバッファ
        Microsoft::WRL::ComPtr<ID3D11Buffer> boneWeightBuffer;   // ボーンウェイトバッファ
        u32                                  vertexCount = 0;    // 頂点数
        u32                                  indexCount  = 0;    // インデックス数
        u32                                  stride      = 0;    // 頂点のストライド（バイト単位）
    };

    //--------------------------------------------------------------
    //! @brief  メッシュデータを作成する関数
    //! @param  device [in] DirectXのデバイス
    //! @param  meshData [in] メッシュデータ
    //! @return メッシュバッファ構造体
    //--------------------------------------------------------------
    [[nodiscard]]
    MeshBuffer CreateMeshBuffer(ID3D11Device* device, const Tsukino::GraphicsCommon::MeshData& meshData);
}    // namespace Tsukino::Renderer

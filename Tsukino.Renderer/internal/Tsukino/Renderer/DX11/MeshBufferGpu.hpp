//----------------------------------------------------------------------------
//! @file   MeshBufferGpu.hpp
//! @brief  MeshBuffer の DX11 の中身（Renderer の内部用）
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Renderer/MeshBuffer.hpp>

#include <d3d11.h>
#include <wrl/client.h>

// 名前空間 : Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    struct MeshData;    // 前方宣言
}    // namespace Tsukino::GraphicsCommon

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! MeshBuffer の GPU の資源です。
    //------------------------------------------------------------------------
    struct MeshBuffer::Gpu {
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;        // 頂点バッファ
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;         // インデックスバッファ
        Microsoft::WRL::ComPtr<ID3D11Buffer> boneWeightBuffer;    // ボーンウェイトバッファ
    };

    //! メッシュデータを GPU へ送り、メッシュバッファを作成します。
    //! @param  [in] device   描画デバイス
    //! @param  [in] meshData メッシュデータ
    //! @return メッシュバッファ（頂点が無いときはバッファの無いもの）
    [[nodiscard]]
    MeshBuffer CreateMeshBuffer(ID3D11Device* device, const Tsukino::GraphicsCommon::MeshData& meshData);
}    // namespace Tsukino::Renderer

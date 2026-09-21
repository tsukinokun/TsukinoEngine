//--------------------------------------------------------------
//! @file   MeshBuffer.hpp
//! @brief  メッシュバッファ構造体の宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
// windows.h の min/max マクロを避けるため、DX11 / Effekseer より先に通す
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <d3d11.h>
#include <hlsl++.h>
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

        //----------------------------------------------------------
        // バウンディング球（ローカル空間）。MeshData の AABB から作る。
        // シャドウパスがカスケードの範囲外を間引くのに使う。
        //
        // AABBではなく球にしてあるのは、ワールド行列で回してもサイズが
        // 変わらないため。AABBのまま回すと軸に沿い直す計算が毎回要る。
        //
        // boundsRadius が 0 のときは「バウンド不明」として扱い、間引かない。
        // プリミティブのように MeshData 側が bounds を埋めていない経路があるため、
        // 既定値のまま誤って消えることが無いようにしている
        //----------------------------------------------------------
        hlslpp::float3 boundsCenter = hlslpp::float3(0.0f, 0.0f, 0.0f);
        float          boundsRadius = 0.0f;
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

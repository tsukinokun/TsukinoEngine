//--------------------------------------------------------------
//! @file   MeshBuffer.hpp
//! @brief  メッシュバッファ構造体の宣言
//! @detail GPU へ送ったメッシュを表します。頂点・インデックスのバッファの実体は
//!         描画 API ごとの中身（MeshBuffer::Gpu）が持ち、Renderer の中でだけ読みます。
//!         作成は RenderResources::CreateMesh() で行います。
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

#include <hlsl++.h>

#include <memory>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @struct MeshBuffer
    //! @brief  メッシュバッファ構造体
    //! @note   コピーすると GPU の資源を共有する（中身は参照カウントで持つ）
    //--------------------------------------------------------------
    struct MeshBuffer {
        struct Gpu;    //!< 描画 API の資源（Renderer の内部で定義）

        std::shared_ptr<Gpu> gpu;                 // 頂点・インデックス・ボーンウェイトのバッファ
        u32                  vertexCount = 0;     // 頂点数
        u32                  indexCount  = 0;     // インデックス数
        u32                  stride      = 0;     // 頂点のストライド（バイト単位）

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
}    // namespace Tsukino::Renderer

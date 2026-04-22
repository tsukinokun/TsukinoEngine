//--------------------------------------------------------------
//! @file   MeshData.hpp
//! @brief  メッシュデータの構造体を定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/GraphicsCommon/Vertex/VertexFormat.hpp>

#include <Tsukino/Core/typedef.hpp>

#include <vector>
#include <cstdint>
// 名前空間 Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    //--------------------------------------------------------------
    //! @struct MeshData
    //! @brief  メッシュデータの構造体
    //--------------------------------------------------------------
    struct MeshData {
        std::vector<u8>  vertexData;    // 生の頂点バイト列
        std::vector<u32> indices;       // 頂点インデックス

        u32 vertexStride = 0;    // 頂点データの1頂点あたりのバイト数
        u32 vertexCount  = 0;    // 頂点の総数
        u32 indexCount   = 0;    // 頂点インデックスの総数

        VertexFormat format = VertexFormat::Unknown;    // 頂点のフォーマット

        //--------------------------------------------------------------
        //! @brief  シリアライズ関数
        //! @tparam Archive シリアライズアーカイブの型
        //! @param  ar [in,out] シリアライズアーカイブ
        //--------------------------------------------------------------
        template <class Archive>
        void serialize(Archive& ar) {
            ar(vertexData, indices, vertexStride, vertexCount, indexCount, format);
        }
    };

}    // namespace Tsukino::GraphicsCommon

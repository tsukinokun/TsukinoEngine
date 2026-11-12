//--------------------------------------------------------------
//! @file   VertexPNUV.hpp
//! @brief  頂点構造体（位置 + 法線 + UV）
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/GraphicsCommon/Vertex/PackedFloat.hpp>

// 名前空間 : Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    //--------------------------------------------------------------
    //! @struct VertexPNUV
    //! @brief  頂点構造体（位置 + 法線 + UV）
    //--------------------------------------------------------------
    struct VertexPNUV {
        Float3 position;    // 頂点の位置（x, y, z）
        Float3 normal;      // 頂点の法線ベクトル（x, y, z）
        Float2 uv;          // 頂点のUV座標（u, v）
    };
}    // namespace Tsukino::GraphicsCommon

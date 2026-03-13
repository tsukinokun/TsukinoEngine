//--------------------------------------------------------------
//! @file   VertexPNUV.hpp
//! @brief  頂点構造体（位置 + 法線 + UV）
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <DirectXMath.h>
// 名前空間 : Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    //--------------------------------------------------------------
    //! @struct VertexPNUV
    //! @brief  頂点構造体（位置 + 法線 + UV）
    //--------------------------------------------------------------
    struct VertexPNUV {
        DirectX::XMFLOAT3 position;    // 頂点の位置（x, y, z）
        DirectX::XMFLOAT3 normal;      // 頂点の法線ベクトル（x, y, z）
        DirectX::XMFLOAT2 uv;          // 頂点のUV座標（u, v）
    };
}    // namespace Tsukino::GraphicsCommon

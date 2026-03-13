//--------------------------------------------------------------
//! @file   VertexPUV.hpp
//! @brief  頂点構造体（位置 + UV）
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <DirectXMath.h>
// 名前空間 : Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    //--------------------------------------------------------------
    //! @struct VertexPUV
    //! @brief  頂点構造体（位置 + UV）
    //--------------------------------------------------------------
    struct VertexPUV {
        DirectX::XMFLOAT3 position;    // 頂点の位置（x, y, z）
        DirectX::XMFLOAT2 uv;          // 頂点のUV座標（u, v）
    };
}    // namespace Tsukino::GraphicsCommon

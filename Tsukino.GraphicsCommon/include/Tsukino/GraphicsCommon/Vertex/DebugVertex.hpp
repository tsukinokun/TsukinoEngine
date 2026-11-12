//--------------------------------------------------------------
//! @file   DebugVertex.hpp
//! @brief  デバッグ描画用の頂点構造体（位置 + カラー）
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/GraphicsCommon/Vertex/PackedFloat.hpp>

// 名前空間 : Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    //--------------------------------------------------------------
    //! @struct DebugVertex
    //! @brief  デバッグ描画用（位置 + カラー）
    //--------------------------------------------------------------
    struct DebugVertex {
        Float3 position;    // 頂点の位置（x, y, z）
        Float4 color;       // 頂点カラー（r, g, b, a）
    };
}    // namespace Tsukino::GraphicsCommon

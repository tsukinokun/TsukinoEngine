//--------------------------------------------------------------
//! @file   PrimitiveType.hpp
//! @brief  メッシュのプリミティブデータを生成する関数の宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include "MeshData.hpp"
// 名前空間 Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    //--------------------------------------------------------------
    //! @enum class PrimitiveType
    //! @brief  プリミティブの種類を表す列挙型
    //--------------------------------------------------------------
    enum class PrimitiveType {
        Quad,              // 四角形
        FullscreenQuad,    // 全画面四角形
        Cube,              // 立方体
        Sphere,            // 球
        Count              // 列挙型の要素数を表す
    };

}    // namespace Tsukino::GraphicsCommon

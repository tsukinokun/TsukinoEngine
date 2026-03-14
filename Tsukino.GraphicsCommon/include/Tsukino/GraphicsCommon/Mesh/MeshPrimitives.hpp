//--------------------------------------------------------------
//! @file   MeshPrimitives.hpp
//! @brief  メッシュのプリミティブデータを生成する関数の宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include "MeshData.hpp"
// 名前空間 Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    //--------------------------------------------------------------
    // 四角形のメッシュデータを生成する関数
    //! @return 四角形のメッシュデータ
    //--------------------------------------------------------------
    [[nodiscard]]
    MeshData CreateQuadMeshData();

}    // namespace Tsukino::GraphicsCommon

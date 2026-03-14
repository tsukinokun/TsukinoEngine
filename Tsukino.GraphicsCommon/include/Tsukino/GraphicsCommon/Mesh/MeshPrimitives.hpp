//--------------------------------------------------------------
//! @file   MeshPrimitives.hpp
//! @brief  メッシュのプリミティブデータを生成する関数の宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/GraphicsCommon/Mesh/MeshData.hpp>
#include <Tsukino/GraphicsCommon/Mesh/PrimitiveType.hpp>
// 名前空間 Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    //--------------------------------------------------------------
    //! @brief  プリミティブの種類を指定してメッシュデータを生成する関数
    //! @param  type [in] プリミティブの種類
    //! @return 指定したプリミティブのメッシュデータ
    //--------------------------------------------------------------
    [[nodiscard]]
    MeshData CreatePrimitiveMeshData(PrimitiveType type);

    //--------------------------------------------------------------
    // 四角形のメッシュデータを生成する関数
    //! @return 四角形のメッシュデータ
    //--------------------------------------------------------------
    [[nodiscard]]
    MeshData CreateQuadMeshData();

}    // namespace Tsukino::GraphicsCommon

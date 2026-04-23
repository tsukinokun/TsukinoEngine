//--------------------------------------------------------------
//! @file   NodeData.hpp
//! @brief  ノードデータの構造体を定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once

#include <Tsukino/Core/typedef.hpp>
#include <hlsl++.h>
#include <string>
#include <vector>

// 名前空間 Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    //--------------------------------------------------------------
    //! @struct NodeData
    //! @brief  モデルのノード（パーツ・ボーン）1つ分のデータ
    //--------------------------------------------------------------
    struct NodeData {
        std::string name;                      // ノード名
        u32         meshIndex = UINT32_MAX;    // 対応する MeshData のインデックス（なし = UINT32_MAX）

        // ローカルトランスフォーム
        hlslpp::interop::float3 translation = hlslpp::float3(0.0f, 0.0f, 0.0f);
        hlslpp::interop::float4 rotation    = hlslpp::float4(0.0f, 0.0f, 0.0f, 1.0f);    // クォータニオン
        hlslpp::interop::float3 scale       = hlslpp::float3(1.0f, 1.0f, 1.0f);

        // 階層構造
        u32              parentIndex = UINT32_MAX;    // 親ノードのインデックス（ルートノード = UINT32_MAX）
        std::vector<u32> childIndices;                // 子ノードのインデックス一覧

        //--------------------------------------------------------------
        //! @brief cereal シリアライズ
        //--------------------------------------------------------------
        template <class Archive>
        void serialize(Archive& ar) {
            ar(name, meshIndex, translation, rotation, scale, parentIndex, childIndices);
        }
    };

}    // namespace Tsukino::GraphicsCommon

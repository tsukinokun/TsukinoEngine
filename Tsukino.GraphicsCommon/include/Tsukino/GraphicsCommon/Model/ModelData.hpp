//--------------------------------------------------------------
//! @file   ModelData.hpp
//! @brief  モデルデータの構造体を定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/GraphicsCommon/Node/NodeData.hpp>
#include <Tsukino/GraphicsCommon/Mesh/MeshData.hpp>
#include <Tsukino/GraphicsCommon/Material/MaterialData.hpp>
#include <vector>

// hlslpp のシリアライズ
namespace hlslpp {
    namespace interop {
        template <class Archive>
        void serialize(Archive& ar, float3& f) {
            ar(f.x, f.y, f.z);
        }
        template <class Archive>
        void serialize(Archive& ar, float4& f) {
            ar(f.x, f.y, f.z, f.w);
        }
    }
}

// 名前空間 Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {

    //--------------------------------------------------------------
    //! @struct ModelData
    //! @brief  モデルデータの構造体
    //--------------------------------------------------------------
    struct ModelData {
        std::vector<NodeData>     nodes;                // ノード一覧
        std::vector<MeshData>     meshes;               // メッシュ一覧
        std::vector<MaterialData> materials;            // マテリアル一覧
        u32                       rootNodeIndex = 0;    // ルートノードのインデックス

        //--------------------------------------------------------------
        //! @brief cereal シリアライズ
        //--------------------------------------------------------------
        template <class Archive>
        void serialize(Archive& ar) {
            ar(nodes, meshes, materials, rootNodeIndex);
        }
    };

}    // namespace Tsukino::GraphicsCommon

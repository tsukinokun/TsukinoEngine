//--------------------------------------------------------------
//! @file   MeshPrimitives.cpp
//! @brief  メッシュのプリミティブデータを生成する関数の定
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/GraphicsCommon/Mesh/MeshPrimitives.hpp>
#include <Tsukino/GraphicsCommon/Vertex/VertexFormat.hpp>
#include <Tsukino/GraphicsCommon/Vertex/VertexPUV.hpp>
// 名前空間 Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    //--------------------------------------------------------------
    //! @brief 四角形のメッシュデータを生成する関数
    //--------------------------------------------------------------
    MeshData CreateQuadMeshData() {
        MeshData mesh;    // メッシュデータ構造体のインスタンスを作成

        // 頂点フォーマットを指定
        mesh.format = VertexFormat::PositionUV;

        // 1頂点のサイズ
        mesh.vertexStride = sizeof(VertexPUV);

        // Quad の頂点データ
        VertexPUV vertices[4] = {
            {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}},
            {{0.5f, -0.5f, 0.0f},  {1.0f, 1.0f}},
            {{0.5f, 0.5f, 0.0f},   {1.0f, 0.0f}},
            {{-0.5f, 0.5f, 0.0f},  {0.0f, 0.0f}},
        };

        mesh.vertexCount = 4;

        // 生バイト列としてコピー
        mesh.vertexData.resize(sizeof(vertices));
        std::memcpy(mesh.vertexData.data(), vertices, sizeof(vertices));

        // インデックス
        mesh.indices    = {0, 1, 2, 2, 3, 0};
        mesh.indexCount = 6;

        return mesh;
    }

}    // namespace Tsukino::GraphicsCommon

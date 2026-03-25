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
    //! @brief  プリミティブの種類を指定してメッシュデータを生成する関数
    //--------------------------------------------------------------
    MeshData CreatePrimitiveMeshData(PrimitiveType type) {
        // プリミティブの種類に対応するメッシュデータ生成関数の型エイリアス
        using PrimitiveCreator = MeshData (*)();

        // プリミティブの種類に対応するメッシュデータ生成関数の配列(ディスパッチテーブル)
        static constexpr PrimitiveCreator creators[(size_t)PrimitiveType::Count] = {
            CreateQuadMeshData,
        };

        // 列挙型の値を配列のインデックスとして使用
        size_t index = (size_t)type;
        if(index >= (size_t)PrimitiveType::Count)
            return {};

        // 対応するメッシュデータ生成関数を呼び出して結果を返す
        return creators[index]();
    }

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
        mesh.indices    = {2, 1, 0, 3, 2, 0};
        mesh.indexCount = 6;

        return mesh;
    }

}    // namespace Tsukino::GraphicsCommon

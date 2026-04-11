//--------------------------------------------------------------
//! @file   ModelImporter.cpp
//! @brief  モデルインポーター
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Model/ModelImporter.hpp>

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <d3dcompiler.h>
#include <fstream>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    // 独自バイナリ用の頂点定義（エンジンの頂点レイアウトに合わせる）
    struct Vertex {
        hlslpp::float3 position;
        hlslpp::float3 normal;
        hlslpp::float2 texcoord;
    };

    // 中間ファイルのヘッダー構造
    struct ModelHeader {
        char magic[4]  = {'T', 'S', 'M', ' '};
        u32  version   = 1;
        u32  meshCount = 0;
    };

    //--------------------------------------------------------------
    //! @brief  モデルアセットをインポートする関数
    //--------------------------------------------------------------
    bool ModelImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        Assimp::Importer importer;

        //--------------------------------------------------------------
        // Assimpでファイルをロード
        //--------------------------------------------------------------
        const aiScene* scene = importer.ReadFile(
            inputPath.string(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_LimitBoneWeights | aiProcess_OptimizeMeshes);

        if(!scene || !scene->mRootNode) {
            Tsukino::Core::Log::Error("Assimp Load Error");
            return false;
        }

        //--------------------------------------------------------------
        // 出力パスの構築 (例: input.fbx -> output/input.tsm)
        //--------------------------------------------------------------
        Tsukino::Core::Path tempPath = inputPath;
        tempPath.replace_extension(".tsm");
        Tsukino::Core::Path outputPath = outputDirectory / tempPath.stem();
        std::ofstream       ofs(outputPath.string(), std::ios::binary);
        if(!ofs) {
            Tsukino::Core::Log::Error("Failed to open output file");
            return false;
        }

        //--------------------------------------------------------------
        // データの解析と書き出し
        //--------------------------------------------------------------
        ModelHeader header;
        header.meshCount = scene->mNumMeshes;
        ofs.write(reinterpret_cast<const char*>(&header), sizeof(ModelHeader));

        for(u32 i = 0; i < scene->mNumMeshes; ++i) {
            const aiMesh* mesh = scene->mMeshes[i];

            // 頂点データの詰め替え
            std::vector<Vertex> vertices(mesh->mNumVertices);
            for(u32 v = 0; v < mesh->mNumVertices; ++v) {
                vertices[v].position = {mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z};
                vertices[v].normal   = {mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z};
                if(mesh->HasTextureCoords(0)) {
                    vertices[v].texcoord = {mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y};
                }
            }

            // インデックスデータの詰め替え
            std::vector<u32> indices;
            for(u32 f = 0; f < mesh->mNumFaces; ++f) {
                const aiFace& face = mesh->mFaces[f];
                for(u32 idx = 0; idx < face.mNumIndices; ++idx) {
                    indices.push_back(face.mIndices[idx]);
                }
            }

            // メッシュごとの情報を書き出し
            u32 vCount = static_cast<u32>(vertices.size());
            u32 iCount = static_cast<u32>(indices.size());

            ofs.write(reinterpret_cast<const char*>(&vCount), sizeof(u32));
            ofs.write(reinterpret_cast<const char*>(vertices.data()), sizeof(Vertex) * vCount);

            ofs.write(reinterpret_cast<const char*>(&iCount), sizeof(u32));
            ofs.write(reinterpret_cast<const char*>(indices.data()), sizeof(u32) * iCount);
        }

        Tsukino::Core::Log::Error("Successfully imported model");
        return true;
    }

}    // namespace Tsukino::Asset

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
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <string>
#include <vector>
#include <functional>

#include <Tsukino/GraphicsCommon/Model/ModelData.hpp>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    // 独自バイナリ用の頂点定義（エンジンの頂点レイアウトに合わせる）
    struct Vertex {
        hlslpp::float3 position;
        hlslpp::float3 normal;
        hlslpp::float2 texcoord;
    };
    
    //--------------------------------------------------------------
    //! @brief  モデルアセットをインポートする関数
    //--------------------------------------------------------------
    bool ModelImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        //--------------------------------------------------------------
        // 拡張子チェック
        //--------------------------------------------------------------
        std::string ext = inputPath.extension();
        // 小文字に変換して判定しやすくする
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // 主要な3Dモデル形式以外は弾く
        if(ext != ".fbx" && ext != ".obj" && ext != ".gltf" && ext != ".glb" && ext != ".dae") {
            Tsukino::Core::Log::Error("Unsupported model format");
            return false;
        }

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
        Tsukino::Core::Path outputPath = outputDirectory / tempPath;

        //--------------------------------------------------------------
        // 親ディレクトリ作成
        //--------------------------------------------------------------
        Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path());

        std::ofstream       ofs(outputPath.string(), std::ios::binary);
        if(!ofs) {
            Tsukino::Core::Log::Error("Failed to open output file");
            return false;
        }

        //--------------------------------------------------------------
        // データの解析と書き出し
        //--------------------------------------------------------------
        Tsukino::GraphicsCommon::ModelData modelData;

        // メッシュ
        modelData.meshes.resize(scene->mNumMeshes);
        for(u32 i = 0; i < scene->mNumMeshes; ++i) {
            const aiMesh* aiMesh = scene->mMeshes[i];
            auto&         dstMesh = modelData.meshes[i];

            std::vector<Vertex> vertices(aiMesh->mNumVertices);
            for(u32 v = 0; v < aiMesh->mNumVertices; ++v) {
                vertices[v].position = {aiMesh->mVertices[v].x, aiMesh->mVertices[v].y, aiMesh->mVertices[v].z};
                vertices[v].normal   = {aiMesh->mNormals[v].x, aiMesh->mNormals[v].y, aiMesh->mNormals[v].z};
                if(aiMesh->HasTextureCoords(0)) {
                    vertices[v].texcoord = {aiMesh->mTextureCoords[0][v].x, aiMesh->mTextureCoords[0][v].y};
                }
            }

            dstMesh.indices.reserve(aiMesh->mNumFaces * 3);
            for(u32 f = 0; f < aiMesh->mNumFaces; ++f) {
                const aiFace& face = aiMesh->mFaces[f];
                for(u32 idx = 0; idx < face.mNumIndices; ++idx) {
                    dstMesh.indices.push_back(face.mIndices[idx]);
                }
            }

            dstMesh.vertexCount  = static_cast<u32>(vertices.size());
            dstMesh.indexCount   = static_cast<u32>(dstMesh.indices.size());
            dstMesh.vertexStride = sizeof(Vertex);
            dstMesh.format       = Tsukino::GraphicsCommon::VertexFormat::PositionNormalUV; // If defined, else skip

            dstMesh.vertexData.resize(vertices.size() * sizeof(Vertex));
            std::memcpy(dstMesh.vertexData.data(), vertices.data(), dstMesh.vertexData.size());
        }

        // ノード
        if (scene->mRootNode) {
            std::function<u32(const aiNode*, u32)> ProcessNode = [&](const aiNode* aiNode, u32 parentIndex) -> u32 {
                u32 currentIndex = static_cast<u32>(modelData.nodes.size());
                modelData.nodes.emplace_back();

                // 参照を改めて取得 (vector再確保によるポインタ無効化回避)
                // (emplace_back直後はback()で取れるが、再帰呼び出し中にキャパシティ超え再確保が起こると
                // dstNode参照がdanglingになるため、メンバへの書き込みはIDを使ってアクセスするようにする)

                modelData.nodes[currentIndex].name = aiNode->mName.C_Str();
                modelData.nodes[currentIndex].parentIndex = parentIndex;
                if (aiNode->mNumMeshes > 0) {
                    modelData.nodes[currentIndex].meshIndex = aiNode->mMeshes[0];
                } else {
                    modelData.nodes[currentIndex].meshIndex = UINT32_MAX;
                }

                aiVector3D scaling, position;
                aiQuaternion aiRot;
                aiNode->mTransformation.Decompose(scaling, aiRot, position);
                modelData.nodes[currentIndex].translation = hlslpp::float3(position.x, position.y, position.z);
                modelData.nodes[currentIndex].rotation = hlslpp::float4(aiRot.x, aiRot.y, aiRot.z, aiRot.w);
                modelData.nodes[currentIndex].scale = hlslpp::float3(scaling.x, scaling.y, scaling.z);

                for (u32 i = 0; i < aiNode->mNumChildren; ++i) {
                    u32 childIndex = ProcessNode(aiNode->mChildren[i], currentIndex);
                    // ここで再度modelData.nodesが再確保されている可能性があるためcurrentIndexでフェッチしなおす
                    modelData.nodes[currentIndex].childIndices.push_back(childIndex);
                }
                return currentIndex;
            };

            modelData.rootNodeIndex = ProcessNode(scene->mRootNode, UINT32_MAX);
        }

        {
            cereal::BinaryOutputArchive archive(ofs);
            archive(modelData);
        }

        Tsukino::Core::Log::Info("Successfully imported model");
        return true;
    }

}    // namespace Tsukino::Asset

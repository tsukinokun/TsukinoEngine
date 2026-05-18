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
#include <unordered_map>

#include <Tsukino/GraphicsCommon/Model/ModelData.hpp>

namespace Tsukino::Asset {

    struct Vertex {
        hlslpp::float3 position;
        hlslpp::float3 normal;
        hlslpp::float2 texcoord;
    };

    bool ModelImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        //--------------------------------------------------------------
        // 拡張子チェック
        //--------------------------------------------------------------
        std::string ext = inputPath.extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

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
        // 出力パスの構築
        //--------------------------------------------------------------
        Tsukino::Core::Path tempPath = inputPath;
        tempPath.replace_extension(".tsm");
        Tsukino::Core::Path outputPath = outputDirectory / tempPath;

        Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path());

        std::ofstream ofs(outputPath.string(), std::ios::binary);
        if(!ofs) {
            Tsukino::Core::Log::Error("Failed to open output file");
            return false;
        }

        Tsukino::GraphicsCommon::ModelData modelData;

        //--------------------------------------------------------------
        // マテリアル
        //--------------------------------------------------------------
        modelData.materials.resize(scene->mNumMaterials);
        for(u32 i = 0; i < scene->mNumMaterials; ++i) {
            const aiMaterial* aiMat  = scene->mMaterials[i];
            auto&             dstMat = modelData.materials[i];

            // 名前
            aiString matName;
            if(aiMat->Get(AI_MATKEY_NAME, matName) == AI_SUCCESS) {
                dstMat.name = matName.C_Str();
            }

            // ベースカラー（ディフューズカラー）
            aiColor4D color;
            if(aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS) {
                dstMat.baseColor = hlslpp::float4(color.r, color.g, color.b, color.a);
            }

            // エミッシブカラー
            aiColor4D emissive;
            if(aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_EMISSIVE, &emissive) == AI_SUCCESS) {
                dstMat.emissive = hlslpp::float3(emissive.r, emissive.g, emissive.b);
            }

            // メタリック・ラフネス
            // メタリック・ラフネス（Assimp 5.0.1 対応）
            float metallic  = 0.0f;
            float roughness = 0.5f;
            aiMat->Get("$mat.metallicFactor", 0, 0, metallic);
            aiMat->Get("$mat.roughnessFactor", 0, 0, roughness);
            dstMat.metallic  = metallic;
            dstMat.roughness = roughness;

            // テクスチャパス（相対パスをそのまま保存）
            aiString texPath;
            if(aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                dstMat.albedoMap = texPath.C_Str();
            }
            if(aiMat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
                dstMat.normalMap = texPath.C_Str();
            }
            if(aiMat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texPath) == AI_SUCCESS) {
                dstMat.metallicRoughnessMap = texPath.C_Str();
            }
            if(aiMat->GetTexture(aiTextureType_EMISSIVE, 0, &texPath) == AI_SUCCESS) {
                dstMat.emissiveMap = texPath.C_Str();
            }
            if(aiMat->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &texPath) == AI_SUCCESS) {
                dstMat.aoMap = texPath.C_Str();
            }
        }

        //--------------------------------------------------------------
        // メッシュ
        //--------------------------------------------------------------
        modelData.meshes.resize(scene->mNumMeshes);
        
        std::unordered_map<std::string, u32> boneNameToIndex;

        for(u32 i = 0; i < scene->mNumMeshes; ++i) {
            const aiMesh* aiMesh  = scene->mMeshes[i];
            auto&         dstMesh = modelData.meshes[i];

            std::vector<Vertex> vertices(aiMesh->mNumVertices);
            for(u32 v = 0; v < aiMesh->mNumVertices; ++v) {
                vertices[v].position = {aiMesh->mVertices[v].x, aiMesh->mVertices[v].y, aiMesh->mVertices[v].z};
                vertices[v].normal   = {aiMesh->mNormals[v].x, aiMesh->mNormals[v].y, aiMesh->mNormals[v].z};
                if(aiMesh->HasTextureCoords(0)) {
                    vertices[v].texcoord = {aiMesh->mTextureCoords[0][v].x, aiMesh->mTextureCoords[0][v].y};
                }
            }

            // ボーンウェイトの初期化
            dstMesh.boneWeights.resize(aiMesh->mNumVertices);
            std::vector<u32> weightCounts(aiMesh->mNumVertices, 0);

            // ボーンの処理
            for(u32 b = 0; b < aiMesh->mNumBones; ++b) {
                const aiBone* aiBone = aiMesh->mBones[b];
                std::string boneName = aiBone->mName.C_Str();

                u32 boneIndex = 0;
                auto it = boneNameToIndex.find(boneName);
                if(it == boneNameToIndex.end()) {
                    boneIndex = static_cast<u32>(modelData.skeleton.bones.size());
                    boneNameToIndex[boneName] = boneIndex;

                    Tsukino::GraphicsCommon::BoneInfo boneInfo;
                    boneInfo.name = boneName;
                    boneInfo.nodeIndex = UINT32_MAX; // 後で解決する

                    const auto& mat = aiBone->mOffsetMatrix;
                    boneInfo.inverseBindPose = hlslpp::float4x4(
                        mat.a1, mat.b1, mat.c1, mat.d1,
                        mat.a2, mat.b2, mat.c2, mat.d2,
                        mat.a3, mat.b3, mat.c3, mat.d3,
                        mat.a4, mat.b4, mat.c4, mat.d4
                    );

                    modelData.skeleton.bones.push_back(boneInfo);
                } else {
                    boneIndex = it->second;
                }

                // ウェイトのセット
                for(u32 w = 0; w < aiBone->mNumWeights; ++w) {
                    const aiVertexWeight& vw = aiBone->mWeights[w];
                    u32 vId = vw.mVertexId;
                    if(weightCounts[vId] < 4) {
                        dstMesh.boneWeights[vId].boneIndices[weightCounts[vId]] = boneIndex;
                        dstMesh.boneWeights[vId].weights[weightCounts[vId]]     = vw.mWeight;
                        weightCounts[vId]++;
                    }
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
            dstMesh.format       = Tsukino::GraphicsCommon::VertexFormat::PositionNormalUV;

            dstMesh.vertexData.resize(vertices.size() * sizeof(Vertex));
            std::memcpy(dstMesh.vertexData.data(), vertices.data(), dstMesh.vertexData.size());
        }

        //--------------------------------------------------------------
        // ノード
        //--------------------------------------------------------------
        if(scene->mRootNode) {
            std::function<u32(const aiNode*, u32)> ProcessNode = [&](const aiNode* aiNode, u32 parentIndex) -> u32 {
                u32 currentIndex = static_cast<u32>(modelData.nodes.size());
                modelData.nodes.emplace_back();

                std::string nodeName = aiNode->mName.C_Str();
                modelData.nodes[currentIndex].name        = nodeName;
                modelData.nodes[currentIndex].parentIndex = parentIndex;
                modelData.nodes[currentIndex].meshIndex   = (aiNode->mNumMeshes > 0) ? aiNode->mMeshes[0] : UINT32_MAX;

                // ボーンのNodeIndexを解決
                auto it = boneNameToIndex.find(nodeName);
                if(it != boneNameToIndex.end()) {
                    modelData.skeleton.bones[it->second].nodeIndex = currentIndex;
                }

                aiVector3D   scaling, position;
                aiQuaternion aiRot;
                aiNode->mTransformation.Decompose(scaling, aiRot, position);
                modelData.nodes[currentIndex].translation = hlslpp::float3(position.x, position.y, position.z);
                modelData.nodes[currentIndex].rotation    = hlslpp::float4(aiRot.x, aiRot.y, aiRot.z, aiRot.w);
                modelData.nodes[currentIndex].scale       = hlslpp::float3(scaling.x, scaling.y, scaling.z);

                for(u32 i = 0; i < aiNode->mNumChildren; ++i) {
                    u32 childIndex = ProcessNode(aiNode->mChildren[i], currentIndex);
                    modelData.nodes[currentIndex].childIndices.push_back(childIndex);
                }
                return currentIndex;
            };

            modelData.rootNodeIndex = ProcessNode(scene->mRootNode, UINT32_MAX);
        }

        //--------------------------------------------------------------
        // アニメーション
        //--------------------------------------------------------------
        if(scene->HasAnimations()) {
            modelData.animations.resize(scene->mNumAnimations);
            for(u32 i = 0; i < scene->mNumAnimations; ++i) {
                const aiAnimation* aiAnim  = scene->mAnimations[i];
                auto&              dstAnim = modelData.animations[i];

                dstAnim.name           = aiAnim->mName.C_Str();
                dstAnim.duration       = static_cast<float>(aiAnim->mDuration);
                dstAnim.ticksPerSecond = static_cast<float>(aiAnim->mTicksPerSecond != 0.0 ? aiAnim->mTicksPerSecond : 25.0);

                dstAnim.channels.resize(aiAnim->mNumChannels);
                for(u32 c = 0; c < aiAnim->mNumChannels; ++c) {
                    const aiNodeAnim* aiChannel  = aiAnim->mChannels[c];
                    auto&             dstChannel = dstAnim.channels[c];

                    dstChannel.nodeName = aiChannel->mNodeName.C_Str();

                    // 位置キー
                    dstChannel.positionKeys.resize(aiChannel->mNumPositionKeys);
                    for(u32 k = 0; k < aiChannel->mNumPositionKeys; ++k) {
                        const auto& key = aiChannel->mPositionKeys[k];
                        dstChannel.positionKeys[k].time  = static_cast<float>(key.mTime);
                        dstChannel.positionKeys[k].value = hlslpp::float3(static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z));
                    }

                    // 回転キー
                    dstChannel.rotationKeys.resize(aiChannel->mNumRotationKeys);
                    for(u32 k = 0; k < aiChannel->mNumRotationKeys; ++k) {
                        const auto& key = aiChannel->mRotationKeys[k];
                        dstChannel.rotationKeys[k].time  = static_cast<float>(key.mTime);
                        dstChannel.rotationKeys[k].value = hlslpp::float4(static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z), static_cast<float>(key.mValue.w));
                    }

                    // スケールキー
                    dstChannel.scaleKeys.resize(aiChannel->mNumScalingKeys);
                    for(u32 k = 0; k < aiChannel->mNumScalingKeys; ++k) {
                        const auto& key = aiChannel->mScalingKeys[k];
                        dstChannel.scaleKeys[k].time  = static_cast<float>(key.mTime);
                        dstChannel.scaleKeys[k].value = hlslpp::float3(static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z));
                    }
                }
            }
        }

        //--------------------------------------------------------------
        // シリアライズして書き出し
        //--------------------------------------------------------------
        {
            cereal::BinaryOutputArchive archive(ofs);
            archive(modelData);
        }

        Tsukino::Core::Log::Info("Successfully imported model");
        return true;
    }

}    // namespace Tsukino::Asset

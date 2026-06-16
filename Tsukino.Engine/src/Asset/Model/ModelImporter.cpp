//--------------------------------------------------------------
//! @file   ModelImporter.cpp
//! @brief  モデルインポーター
//! @author 山﨑愛
//--------------------------------------------------------------
#define NOMINMAX

#include <Tsukino/Engine/Asset/Model/ModelImporter.hpp>
#include <cstring>

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <DirectXTex/DirectXTex.h>
#include <wincodec.h>

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
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {

    //--------------------------------------------------------------
    //! @brief  頂点データ構造体
    //--------------------------------------------------------------
    struct Vertex {
        hlslpp::interop::float3 position;
        hlslpp::interop::float3 normal;
        hlslpp::interop::float2 texcoord;
    };

    //--------------------------------------------------------------
    //! @brief  モデルのインポート関数
    //--------------------------------------------------------------
    bool ModelImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        Tsukino::Core::Log::Info("sizeof(Vertex) = " + std::to_string(sizeof(Vertex)));
        Tsukino::Core::Log::Info("offsetof texcoord = " + std::to_string(offsetof(Vertex, texcoord)));

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
            Tsukino::Core::Log::Error(importer.GetErrorString());
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
        // テクスチャの数をログ出力
        //--------------------------------------------------------------
        Tsukino::Core::Log::Info("mNumTextures: " + std::to_string(scene->mNumTextures));

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
        };

        //--------------------------------------------------------------
        // 埋め込みテクスチャをDDSに変換・保存
        //--------------------------------------------------------------
        std::string                          modelBaseName = inputPath.stem();
        std::unordered_map<u32, std::string> embeddedTexIndexToDDSPath;

        //--------------------------------------------------------------
        // どのテクスチャインデックスがsRGBか事前に収集
        //--------------------------------------------------------------
        std::unordered_set<u32> srgbTexIndices;

        for(u32 i = 0; i < scene->mNumMaterials; ++i) {
            const aiMaterial* mat = scene->mMaterials[i];

            // sRGBで扱うべきテクスチャタイプ
            constexpr aiTextureType srgbTypes[] = {
                aiTextureType_DIFFUSE,
                aiTextureType_EMISSIVE,
            };

            for(aiTextureType type : srgbTypes) {
                aiString texPath;
                if(mat->GetTexture(type, 0, &texPath) != AI_SUCCESS)
                    continue;

                const aiTexture* embedded = scene->GetEmbeddedTexture(texPath.C_Str());
                if(!embedded)
                    continue;

                for(u32 t = 0; t < scene->mNumTextures; ++t) {
                    if(scene->mTextures[t] == embedded) {
                        srgbTexIndices.insert(t);
                        break;
                    }
                }
            }
        }

        for(u32 i = 0; i < scene->mNumTextures; ++i) {
            const aiTexture* tex = scene->mTextures[i];

            DirectX::ScratchImage image;

            if(tex->mHeight == 0) {
                DirectX::TexMetadata metadata;
                HRESULT              hr = DirectX::LoadFromWICMemory(
                    reinterpret_cast<const std::byte*>(tex->pcData), static_cast<size_t>(tex->mWidth), DirectX::WIC_FLAGS_NONE, &metadata, image, nullptr);
                if(FAILED(hr)) {
                    Tsukino::Core::Log::Error("Failed to load embedded texture: " + std::to_string(i));
                    continue;
                }
            } else {
                HRESULT hr = image.Initialize2D(DXGI_FORMAT_B8G8R8A8_UNORM, tex->mWidth, tex->mHeight, 1, 1);
                if(FAILED(hr)) {
                    Tsukino::Core::Log::Error("Failed to initialize raw texture: " + std::to_string(i));
                    continue;
                }
                const DirectX::Image* img = image.GetImage(0, 0, 0);
                if(!img) {
                    Tsukino::Core::Log::Error("GetImage returned nullptr after Initialize2D: " + std::to_string(i));
                    continue;
                }
                std::memcpy(img->pixels, tex->pcData, img->slicePitch);
            }

            const DirectX::Image* srcImg = image.GetImage(0, 0, 0);
            if(!srcImg) {
                Tsukino::Core::Log::Error("GetImage returned nullptr: " + std::to_string(i));
                continue;
            }

            //--------------------------------------------------------------
            // sRGBかどうかでターゲットフォーマットを決定
            //--------------------------------------------------------------
            const bool isSRGB = srgbTexIndices.count(i) > 0;

            //元データがsRGBなら、メタデータのフォーマットを明示的に_SRGBに書き換える
            if(isSRGB) {
                // ロードされたフォーマットがUNORM系なら、対応する_SRGBフォーマットに上書きする
                DXGI_FORMAT srgbFormat = DirectX::MakeSRGB(image.GetMetadata().format);
                if(srgbFormat != DXGI_FORMAT_UNKNOWN) {
                    image.OverrideFormat(srgbFormat);
                }
            }

            // 中間フォーマットもsRGB / リニアを切り替えるようにする
            const DXGI_FORMAT intermediateFormat = isSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
            const DXGI_FORMAT bcFormat           = isSRGB ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;

            // intermediateFormat（_SRGB か _UNORM）を基準にコンバート
            if(image.GetMetadata().format != intermediateFormat) {
                DirectX::ScratchImage converted;
                HRESULT               hrConv =
                    DirectX::Convert(*image.GetImage(0, 0, 0), intermediateFormat, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, converted);
                if(FAILED(hrConv)) {
                    Tsukino::Core::Log::Error("Failed to convert texture format: " + std::to_string(i));
                    continue;
                }
                image = std::move(converted);
            }

            // 2048を超える場合はリサイズ
            constexpr size_t maxSize = 2048;
            if(image.GetMetadata().width > maxSize || image.GetMetadata().height > maxSize) {
                size_t                newWidth  = std::min(image.GetMetadata().width, maxSize);
                size_t                newHeight = std::min(image.GetMetadata().height, maxSize);
                DirectX::ScratchImage resized;
                DirectX::Resize(*image.GetImage(0, 0, 0), newWidth, newHeight, DirectX::TEX_FILTER_DEFAULT, resized);
                image = std::move(resized);
            }

            // 4の倍数にリサイズ（BC圧縮の要件）
            {
                size_t newWidth  = (image.GetMetadata().width + 3) & ~3;
                size_t newHeight = (image.GetMetadata().height + 3) & ~3;
                if(newWidth != image.GetMetadata().width || newHeight != image.GetMetadata().height) {
                    DirectX::ScratchImage resized;
                    DirectX::Resize(*image.GetImage(0, 0, 0), newWidth, newHeight, DirectX::TEX_FILTER_DEFAULT, resized);
                    image = std::move(resized);
                }
            }

            // ミップマップ生成
            {
                DirectX::ScratchImage mipChain;
                HRESULT               hrMip = DirectX::GenerateMipMaps(*image.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, mipChain);
                if(FAILED(hrMip)) {
                    Tsukino::Core::Log::Error("Failed to generate mipmaps: " + std::to_string(i));
                    continue;
                }
                image = std::move(mipChain);
            }

            // BC3圧縮（sRGB / リニアを自動選択）
            DirectX::ScratchImage compressed;
            HRESULT               hr = DirectX::Compress(image.GetImages(),
                                           image.GetImageCount(),
                                           image.GetMetadata(),
                                           bcFormat,
                                           DirectX::TEX_COMPRESS_DEFAULT,
                                           DirectX::TEX_THRESHOLD_DEFAULT,
                                           compressed);
            if(FAILED(hr)) {
                Tsukino::Core::Log::Error("Failed to compress texture: " + std::to_string(i));
                continue;
            }

            // 保存
            std::string         ddsFilename = modelBaseName + "_" + std::to_string(i) + ".dds";
            Tsukino::Core::Path tsmDir      = (outputDirectory / inputPath).parent_path();
            Tsukino::Core::Path ddsPath     = tsmDir / ddsFilename;

            hr = DirectX::SaveToDDSFile(
                compressed.GetImages(), compressed.GetImageCount(), compressed.GetMetadata(), DirectX::DDS_FLAGS_NONE, ddsPath.ToWString().c_str());
            if(FAILED(hr)) {
                Tsukino::Core::Log::Error("Failed to save DDS: " + ddsFilename);
                continue;
            }

            embeddedTexIndexToDDSPath[i] = ddsPath.string();
        }

        auto GetTexPath = [&](const aiMaterial* mat, aiTextureType type) -> std::string {
            aiString texPath;
            if(mat->GetTexture(type, 0, &texPath) != AI_SUCCESS)
                return "";

            const aiTexture* embedded = scene->GetEmbeddedTexture(texPath.C_Str());
            if(!embedded)
                return texPath.C_Str();

            for(u32 t = 0; t < scene->mNumTextures; ++t) {
                if(scene->mTextures[t] == embedded) {
                    auto it = embeddedTexIndexToDDSPath.find(t);
                    if(it != embeddedTexIndexToDDSPath.end())
                        return it->second;
                }
            }
            return "";
        };

        for(u32 i = 0; i < scene->mNumMaterials; ++i) {
            const aiMaterial* aiMat  = scene->mMaterials[i];
            auto&             dstMat = modelData.materials[i];

            dstMat.albedoMap            = GetTexPath(aiMat, aiTextureType_DIFFUSE);
            dstMat.normalMap            = GetTexPath(aiMat, aiTextureType_NORMALS);
            dstMat.metallicRoughnessMap = GetTexPath(aiMat, aiTextureType_DIFFUSE_ROUGHNESS);
            dstMat.emissiveMap          = GetTexPath(aiMat, aiTextureType_EMISSIVE);
            dstMat.aoMap                = GetTexPath(aiMat, aiTextureType_AMBIENT_OCCLUSION);
        }

        //--------------------------------------------------------------
        // メッシュ
        //--------------------------------------------------------------
        modelData.meshes.resize(scene->mNumMeshes);

        std::unordered_map<std::string, u32> boneNameToIndex;

        for(u32 i = 0; i < scene->mNumMeshes; ++i) {
            const aiMesh* aiMesh  = scene->mMeshes[i];
            auto&         dstMesh = modelData.meshes[i];

            dstMesh.materialIndex = aiMesh->mMaterialIndex;

            std::vector<Vertex> vertices(aiMesh->mNumVertices);
            for(u32 v = 0; v < aiMesh->mNumVertices; ++v) {
                vertices[v].position.x = aiMesh->mVertices[v].x;
                vertices[v].position.y = aiMesh->mVertices[v].y;
                vertices[v].position.z = aiMesh->mVertices[v].z;

                vertices[v].normal.x = aiMesh->mNormals[v].x;
                vertices[v].normal.y = aiMesh->mNormals[v].y;
                vertices[v].normal.z = aiMesh->mNormals[v].z;

                if(aiMesh->HasTextureCoords(0)) {
                    vertices[v].texcoord.x = aiMesh->mTextureCoords[0][v].x;
                    vertices[v].texcoord.y = aiMesh->mTextureCoords[0][v].y;
                }
            }

            // ボーンウェイトの初期化
            dstMesh.boneWeights.resize(aiMesh->mNumVertices);
            std::vector<u32> weightCounts(aiMesh->mNumVertices, 0);

            // ボーンの処理

            for(u32 b = 0; b < aiMesh->mNumBones; ++b) {
                const aiBone* aiBone   = aiMesh->mBones[b];
                std::string   boneName = aiBone->mName.C_Str();

                Tsukino::Core::Log::Info("bone count = " + std::to_string(modelData.skeleton.bones.size()));

                u32  boneIndex = 0;
                auto it        = boneNameToIndex.find(boneName);
                if(it == boneNameToIndex.end()) {
                    boneIndex                 = static_cast<u32>(modelData.skeleton.bones.size());
                    boneNameToIndex[boneName] = boneIndex;

                    Tsukino::GraphicsCommon::BoneInfo boneInfo;
                    boneInfo.name      = boneName;
                    boneInfo.nodeIndex = UINT32_MAX;    // 後で解決する

                    const auto& mat          = aiBone->mOffsetMatrix;
                    boneInfo.inverseBindPose = hlslpp::float4x4(
                        mat.a1, mat.b1, mat.c1, mat.d1, mat.a2, mat.b2, mat.c2, mat.d2, mat.a3, mat.b3, mat.c3, mat.d3, mat.a4, mat.b4, mat.c4, mat.d4);

                    modelData.skeleton.bones.push_back(boneInfo);
                } else {
                    boneIndex = it->second;
                }

                // ウェイトのセット
                for(u32 w = 0; w < aiBone->mNumWeights; ++w) {
                    const aiVertexWeight& vw  = aiBone->mWeights[w];
                    u32                   vId = vw.mVertexId;
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

                std::string nodeName                      = aiNode->mName.C_Str();
                modelData.nodes[currentIndex].name        = nodeName;
                modelData.nodes[currentIndex].parentIndex = parentIndex;

                for(u32 i = 0; i < aiNode->mNumMeshes; ++i) {
                    modelData.nodes[currentIndex].meshIndices.push_back(aiNode->mMeshes[i]);
                }

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
                        const auto& key                 = aiChannel->mPositionKeys[k];
                        dstChannel.positionKeys[k].time = static_cast<float>(key.mTime);
                        dstChannel.positionKeys[k].value =
                            hlslpp::float3(static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z));
                    }

                    // 回転キー
                    dstChannel.rotationKeys.resize(aiChannel->mNumRotationKeys);
                    for(u32 k = 0; k < aiChannel->mNumRotationKeys; ++k) {
                        const auto& key                  = aiChannel->mRotationKeys[k];
                        dstChannel.rotationKeys[k].time  = static_cast<float>(key.mTime);
                        dstChannel.rotationKeys[k].value = hlslpp::float4(static_cast<float>(key.mValue.x),
                                                                          static_cast<float>(key.mValue.y),
                                                                          static_cast<float>(key.mValue.z),
                                                                          static_cast<float>(key.mValue.w));
                    }

                    // スケールキー
                    dstChannel.scaleKeys.resize(aiChannel->mNumScalingKeys);
                    for(u32 k = 0; k < aiChannel->mNumScalingKeys; ++k) {
                        const auto& key              = aiChannel->mScalingKeys[k];
                        dstChannel.scaleKeys[k].time = static_cast<float>(key.mTime);
                        dstChannel.scaleKeys[k].value =
                            hlslpp::float3(static_cast<float>(key.mValue.x), static_cast<float>(key.mValue.y), static_cast<float>(key.mValue.z));
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

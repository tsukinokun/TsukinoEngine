//--------------------------------------------------------------
//! @file   ModelImporter.cpp
//! @brief  モデルインポーター
//! @author 山﨑愛
//--------------------------------------------------------------
#define NOMINMAX

#include <Tsukino/Engine/Asset/Model/ModelImporter.hpp>
#include <cstring>
#include <cwchar>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

#ifdef _DEBUG
    //--------------------------------------------------------------
    //! @brief  Assimp デバッグ DLL の既知の誤アサート対策
    //!
    //! Assimp 5.0.1 のデバッグ DLL は、正常に読める FBX に対しても
    //! FBXConverter.cpp:806 の
    //!   ai_assert(NeedsComplexTransformationChain(model) == ((chainBits & chainMaskComplex) != 0))
    //! を誤発火させる。NeedsComplexTransformationChain() 側と
    //! 変換チェーン構築側とで、変換要素が「単位元かどうか」を判定する
    //! 閾値の扱いが食い違っているためで、Assimp 側の既知の不具合である。
    //!
    //! assert が鳴ると _wassert がそのまま abort() を呼ぶためプロセスごと落ちる。
    //! Release DLL は NDEBUG ビルドで ai_assert が消えるため何も起きず、
    //! しかも読み込み結果は Debug/Release で同一（メッシュ・ボーン・
    //! アニメーションのいずれも一致することを確認済み）。
    //! つまり Debug ビルドでのみ、正しいアセットが読めずに落ちていた。
    //!
    //! ここでは assimp DLL のインポートテーブル上の _wassert だけを差し替え、
    //! この既知の1件だけを握り潰して Release と同じ挙動にそろえる。
    //! 他のアサートは本物の _wassert へそのまま流すので握り潰されない。
    //!
    //! @note Assimp を 5.1 以降へ更新できればこの回避策は不要になる。
    //--------------------------------------------------------------
    namespace {
        //! Assimp が置かれているデバッグ DLL の名前
        constexpr const char* kAssimpDebugModuleName = "assimp-vc142-mtd.dll";

        using WassertFn = void(__cdecl*)(wchar_t const*, wchar_t const*, unsigned);

        //! 差し替え前の本物の _wassert
        WassertFn g_originalWassert = nullptr;

        //--------------------------------------------------------------
        //! @brief  assimp から呼ばれる _wassert の差し替え先
        //--------------------------------------------------------------
        void __cdecl AssimpWassertFilter(wchar_t const* expression, wchar_t const* file, unsigned line) {
            const bool isKnownFbxConverterAssert = expression != nullptr && file != nullptr
                                                   && std::wcsstr(expression, L"NeedsComplexTransformationChain") != nullptr
                                                   && std::wcsstr(file, L"FBXConverter.cpp") != nullptr;

            if(isKnownFbxConverterAssert) {
                Tsukino::Core::Log::Warn("Suppressed known Assimp debug assert (FBXConverter.cpp:" + std::to_string(line) + ")");
                return;
            }

            // 想定外のアサートは握り潰さずに本来の挙動へ戻す
            if(g_originalWassert)
                g_originalWassert(expression, file, line);
        }

        //--------------------------------------------------------------
        //! @brief  指定モジュールのインポートテーブルの1関数を差し替える
        //! @param  module      [in] 書き換える対象のモジュール
        //! @param  fromDll     [in] インポート元 DLL 名
        //! @param  functionName[in] 差し替える関数名
        //! @param  replacement [in] 差し替え先
        //! @return 差し替えられたら true
        //--------------------------------------------------------------
        bool PatchImportedFunction(HMODULE module, const char* fromDll, const char* functionName, void* replacement, void** outOriginal) {
            if(!module)
                return false;

            auto* base = reinterpret_cast<BYTE*>(module);
            auto* dos  = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
            if(dos->e_magic != IMAGE_DOS_SIGNATURE)
                return false;

            auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
            if(nt->Signature != IMAGE_NT_SIGNATURE)
                return false;

            const IMAGE_DATA_DIRECTORY& importDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
            if(importDir.VirtualAddress == 0)
                return false;

            for(auto* desc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + importDir.VirtualAddress); desc->Name != 0; ++desc) {
                if(_stricmp(reinterpret_cast<const char*>(base + desc->Name), fromDll) != 0)
                    continue;

                auto* nameThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(base + desc->OriginalFirstThunk);
                auto* addrThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(base + desc->FirstThunk);

                for(; nameThunk->u1.AddressOfData != 0; ++nameThunk, ++addrThunk) {
                    if(IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal))
                        continue;    // 序数インポートは名前で照合できない

                    auto* importByName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + nameThunk->u1.AddressOfData);
                    if(std::strcmp(importByName->Name, functionName) != 0)
                        continue;

                    DWORD oldProtect = 0;
                    if(!::VirtualProtect(addrThunk, sizeof(*addrThunk), PAGE_READWRITE, &oldProtect))
                        return false;

                    if(outOriginal)
                        *outOriginal = reinterpret_cast<void*>(addrThunk->u1.Function);
                    addrThunk->u1.Function = reinterpret_cast<ULONGLONG>(replacement);

                    ::VirtualProtect(addrThunk, sizeof(*addrThunk), oldProtect, &oldProtect);
                    return true;
                }
            }
            return false;
        }

        //--------------------------------------------------------------
        //! @brief  差し替えを一度だけ行う
        //--------------------------------------------------------------
        void EnsureAssimpAssertWorkaround() {
            static const bool installed = [] {
                void*         original = nullptr;
                const HMODULE assimp   = ::GetModuleHandleA(kAssimpDebugModuleName);
                const bool    ok = PatchImportedFunction(assimp, "ucrtbased.dll", "_wassert", reinterpret_cast<void*>(&AssimpWassertFilter), &original);
                if(ok) {
                    g_originalWassert = reinterpret_cast<WassertFn>(original);
                } else {
                    // 差し替えられなくても致命的ではない（該当 FBX で落ちるだけ）ので警告に留める
                    Tsukino::Core::Log::Warn("Failed to install the Assimp debug-assert workaround.");
                }
                return ok;
            }();
            (void)installed;
        }
    }    // namespace
#endif    // _DEBUG

    //--------------------------------------------------------------
    //! @brief  モデルのインポート関数
    //--------------------------------------------------------------
    bool ModelImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
#ifdef _DEBUG
        EnsureAssimpAssertWorkaround();
#endif    // _DEBUG

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
        // (inputPathが絶対パスの場合はToEngineRelativePath()で相対パスに戻してから結合する)
        //--------------------------------------------------------------
        Tsukino::Core::Path tempPath = Tsukino::IO::FileSystem::ToEngineRelativePath(inputPath);
        tempPath.replace_extension(".tsm");
        Tsukino::Core::Path outputPath = outputDirectory / tempPath;

        // 出力先を作れないまま書き込みへ進むと、失敗が原因から遠い場所で
        // 「キャッシュが無い」として現れるため、ここで止める
        if(!Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path())) {
            Tsukino::Core::Log::Error("ModelImporter: Failed to create the output directory: "
                                      + outputPath.parent_path().string());
            return false;
        }

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

            // ShadingModelをマテリアル名のプレフィックスから判定
            if(dstMat.name.rfind("Water_", 0) == 0) {
                dstMat.shadingModel = Tsukino::GraphicsCommon::ShadingModel::Water;
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
            std::string         ddsFilename        = modelBaseName + "_" + std::to_string(i) + ".dds";
            Tsukino::Core::Path relativeInputPath   = Tsukino::IO::FileSystem::ToEngineRelativePath(inputPath);
            Tsukino::Core::Path tsmDir              = (outputDirectory / relativeInputPath).parent_path();
            Tsukino::Core::Path ddsPath             = tsmDir / ddsFilename;

            hr = DirectX::SaveToDDSFile(
                compressed.GetImages(), compressed.GetImageCount(), compressed.GetMetadata(), DirectX::DDS_FLAGS_NONE, ddsPath.ToWString().c_str());
            if(FAILED(hr)) {
                Tsukino::Core::Log::Error("Failed to save DDS: " + ddsFilename);
                continue;
            }

            // マテリアルに焼き込む参照パスは、他のゲームアセットと同じ「GetAssetRootPath()からの
            // 相対パス(Cache/を含まない)」規約にする。AssetManager::Load()が"Cache/"を付与して
            // 実ファイル(ddsPathと同じ場所)を解決する。
            // (ddsPathのような絶対パス、かつ既にCache/を含むパスをそのまま焼き込むと、
            //  AssetManager::Load()経由で再度Cache/が付与されてしまい、二重になったパスが
            //  実在せずロードに失敗する。特にRelease構成ではGetEngineAssetRootPath()と
            //  GetAssetRootPath()が同じディレクトリを指すため、この問題が顕在化する)
            Tsukino::Core::Path materialRefPath = relativeInputPath.parent_path() / ddsFilename;
            embeddedTexIndexToDDSPath[i]        = materialRefPath.string();
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

                Tsukino::Core::Log::Info("Debug: Bone[" + std::to_string(b) + "] Name: " + boneName + ", Weights: " + std::to_string(aiBone->mNumWeights));

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

            // 正規化
            for(u32 v = 0; v < aiMesh->mNumVertices; ++v) {
                auto& bw  = dstMesh.boneWeights[v];
                float sum = bw.weights[0] + bw.weights[1] + bw.weights[2] + bw.weights[3];

                if(sum > 1e-6f) {
                    float invSum   = 1.0f / sum;
                    bw.weights[0] *= invSum;
                    bw.weights[1] *= invSum;
                    bw.weights[2] *= invSum;
                    bw.weights[3] *= invSum;
                } else if(aiMesh->mNumBones > 0) {
                    // どのボーンにも紐付いていない頂点への保険
                    bw.boneIndices[0] = 0;
                    bw.weights[0]     = 1.0f;
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

            if(aiMesh->mNumBones > 0) {
                // 骨があるメッシュなら Skinned にする
                dstMesh.format = Tsukino::GraphicsCommon::VertexFormat::Skinned;
            } else {
                // 骨がないメッシュなら通常の 3D メッシュ
                dstMesh.format = Tsukino::GraphicsCommon::VertexFormat::PositionNormalUV;
            }

            dstMesh.vertexData.resize(vertices.size() * sizeof(Vertex));
            std::memcpy(dstMesh.vertexData.data(), vertices.data(), dstMesh.vertexData.size());
        }

        //--------------------------------------------------------------
        // ノード
        //--------------------------------------------------------------
        std::vector<hlslpp::float4x4> nodeWorldMatrices;    // ノードごとのワールド行列（indexはmodelData.nodesと対応）

        if(scene->mRootNode) {
            std::function<u32(const aiNode*, u32, const hlslpp::float4x4&)> ProcessNode =
                [&](const aiNode* aiNode, u32 parentIndex, const hlslpp::float4x4& parentWorld) -> u32 {
                u32 currentIndex = static_cast<u32>(modelData.nodes.size());
                modelData.nodes.emplace_back();
                nodeWorldMatrices.emplace_back();    // ← 追加：nodeWorldMatricesもノードと1:1で増やす

                std::string nodeName                      = aiNode->mName.C_Str();
                modelData.nodes[currentIndex].name        = nodeName;
                modelData.nodes[currentIndex].parentIndex = parentIndex;

                for(u32 i = 0; i < aiNode->mNumMeshes; ++i) {
                    modelData.nodes[currentIndex].meshIndices.push_back(aiNode->mMeshes[i]);
                }

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

                const aiMatrix4x4& m = aiNode->mTransformation;
                hlslpp::float4x4   localMat(m.a1, m.a2, m.a3, m.a4, m.b1, m.b2, m.b3, m.b4, m.c1, m.c2, m.c3, m.c4, m.d1, m.d2, m.d3, m.d4);
                hlslpp::float4x4   worldMat     = hlslpp::mul(localMat, parentWorld);
                nodeWorldMatrices[currentIndex] = worldMat;    // ← ここで初めて実際の行列を格納

                for(u32 i = 0; i < aiNode->mNumChildren; ++i) {
                    u32 childIndex = ProcessNode(aiNode->mChildren[i], currentIndex, worldMat);    // ← worldMatを子に渡す
                    modelData.nodes[currentIndex].childIndices.push_back(childIndex);
                }
                return currentIndex;
            };

            modelData.rootNodeIndex = ProcessNode(scene->mRootNode, UINT32_MAX, hlslpp::float4x4::identity());
        }

        //--------------------------------------------------------------
        // メッシュごとのワールド空間AABBを再計算
        // （どのノードがそのメッシュを参照しているかを逆引きする）
        //--------------------------------------------------------------
        std::vector<hlslpp::float3> meshMinBound(scene->mNumMeshes, hlslpp::float3(FLT_MAX, FLT_MAX, FLT_MAX));
        std::vector<hlslpp::float3> meshMaxBound(scene->mNumMeshes, hlslpp::float3(-FLT_MAX, -FLT_MAX, -FLT_MAX));

        for(u32 nodeIdx = 0; nodeIdx < modelData.nodes.size(); ++nodeIdx) {
            for(u32 meshIdx : modelData.nodes[nodeIdx].meshIndices) {
                const aiMesh* aiMesh = scene->mMeshes[meshIdx];
                for(u32 v = 0; v < aiMesh->mNumVertices; ++v) {
                    hlslpp::float3 localPos(aiMesh->mVertices[v].x, aiMesh->mVertices[v].y, aiMesh->mVertices[v].z);
                    hlslpp::float4 worldPos4 = hlslpp::mul(hlslpp::float4(localPos, 1.0f), nodeWorldMatrices[nodeIdx]);
                    hlslpp::float3 worldPos(worldPos4.x, worldPos4.y, worldPos4.z);

                    meshMinBound[meshIdx] = hlslpp::min(meshMinBound[meshIdx], worldPos);
                    meshMaxBound[meshIdx] = hlslpp::max(meshMaxBound[meshIdx], worldPos);
                }
            }
        }

        for(u32 i = 0; i < scene->mNumMeshes; ++i) {
            modelData.meshes[i].bounds.min = meshMinBound[i];
            modelData.meshes[i].bounds.max = meshMaxBound[i];
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

//--------------------------------------------------------------
//! @file   ModelLoader.cpp
//! @brief  モデルアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Model/ModelLoader.hpp>
#include <Tsukino/Engine/Asset/Model/ModelAsset.hpp>
#include <Tsukino/Core/Log.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    // ローダー側で扱う最小限の頂点レイアウト（ModelImporter の書き出しと一致させる）
    struct VertexBinary {
        float px, py, pz;
        float nx, ny, nz;
        float u, v;
    };

    // ファイルヘッダ（ModelImporter と互換）
    struct ModelHeaderOnDisk {
        char     magic[4];
        u32 version;
        u32 meshCount;
    };

    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する関数
    //--------------------------------------------------------------
    bool ModelLoader::CanLoad(const std::string& ext) const {
        // 他のローダーと同様に単純比較
        return ext == ".tsm";
    }

    //--------------------------------------------------------------
    //! @brief モデルファイルを読み込み ModelAsset を生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> ModelLoader::Load(const Tsukino::Core::Path& path) {
        const std::string filePath = path.string();

        std::ifstream ifs(filePath, std::ios::binary);
        if(!ifs.is_open()) {
            Tsukino::Core::Log::Error("ModelLoader: Failed to open model: " + filePath);
            return nullptr;
        }

        // ファイルサイズ確認
        ifs.seekg(0, std::ios::end);
        const std::streamoff fileSize = ifs.tellg();
        if(fileSize <= 0) {
            Tsukino::Core::Log::Error("ModelLoader: Empty or invalid file: " + filePath);
            return nullptr;
        }
        ifs.seekg(0, std::ios::beg);

        // ヘッダ読み込み
        ModelHeaderOnDisk header{};
        ifs.read(reinterpret_cast<char*>(&header), sizeof(ModelHeaderOnDisk));
        if(!ifs) {
            Tsukino::Core::Log::Error("ModelLoader: Failed to read header: " + filePath);
            return nullptr;
        }

        // magic チェック
        if(!(header.magic[0] == 'T' && header.magic[1] == 'S' && header.magic[2] == 'M' && header.magic[3] == ' ')) {
            Tsukino::Core::Log::Error("ModelLoader: Invalid magic (not a .tsm): " + filePath);
            return nullptr;
        }

        if(header.version == 0) {
            Tsukino::Core::Log::Warn("ModelLoader: Unknown model version (0) in: " + filePath);
        }

        // ModelAsset を作成（頂点/インデックスの実データは MeshAsset に持たせる設計が望ましいが、
        // ローダーは単独で返すアセットのみ生成できるため、ここではノード情報を生成して返す）
        Tsukino::Core::Ref<ModelAsset> asset = Tsukino::Core::CreateRef<ModelAsset>();

        // mesh ごとにノード（簡易）を作成する
        for(u32 meshIdx = 0; meshIdx < header.meshCount; ++meshIdx) {
            // 頂点数読み取り
            u32 vCount = 0;
            ifs.read(reinterpret_cast<char*>(&vCount), sizeof(u32));
            if(!ifs) {
                Tsukino::Core::Log::Error("ModelLoader: Failed to read vertex count for mesh " + std::to_string(meshIdx) + ": " + filePath);
                return nullptr;
            }

            // 頂点データはスキップ（ここでは MeshAsset を生成して AssetManager に登録する処理を行わない）
            const std::streamoff vertexBytes = static_cast<std::streamoff>(vCount) * static_cast<std::streamoff>(sizeof(VertexBinary));
            if(vertexBytes < 0) {    // 保険
                Tsukino::Core::Log::Error("ModelLoader: Invalid vertex size for mesh " + std::to_string(meshIdx) + ": " + filePath);
                return nullptr;
            }
            ifs.seekg(vertexBytes, std::ios::cur);
            if(!ifs) {
                Tsukino::Core::Log::Error("ModelLoader: Failed to skip vertex data for mesh " + std::to_string(meshIdx) + ": " + filePath);
                return nullptr;
            }

            // インデックス数読み取り
            u32 iCount = 0;
            ifs.read(reinterpret_cast<char*>(&iCount), sizeof(u32));
            if(!ifs) {
                Tsukino::Core::Log::Error("ModelLoader: Failed to read index count for mesh " + std::to_string(meshIdx) + ": " + filePath);
                return nullptr;
            }

            // インデックスデータをスキップ
            const std::streamoff indexBytes = static_cast<std::streamoff>(iCount) * static_cast<std::streamoff>(sizeof(u32));
            ifs.seekg(indexBytes, std::ios::cur);
            if(!ifs) {
                Tsukino::Core::Log::Error("ModelLoader: Failed to skip index data for mesh " + std::to_string(meshIdx) + ": " + filePath);
                return nullptr;
            }

            // ノード/RenderUnit を作成（簡易：1 メッシュ = 1 ノード、Mesh/Material ハンドルは未割当）
            ModelNode node{};
            node.name = "Mesh_" + std::to_string(meshIdx);
            // transform はデフォルト初期化（必要なら ModelImporter 側で追加情報を書き出す）
            node.parentIndex = -1;

            RenderUnit ru{};
            ru.meshHandle     = AssetHandle::Invalid();
            ru.materialHandle = AssetHandle::Invalid();
            node.renderUnits.push_back(ru);

            asset->nodes.push_back(std::move(node));
        }

        Tsukino::Core::Log::Info("ModelLoader: Successfully loaded .tsm (" + std::to_string(asset->nodes.size()) + " mesh nodes): " + filePath);
        return asset;
    }
}    // namespace Tsukino::Asset

//--------------------------------------------------------------
//! @file   ModelLoader.cpp
//! @brief  モデルアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Model/ModelLoader.hpp>
#include <Tsukino/Engine/Asset/Model/ModelAsset.hpp>
#include <Tsukino/Engine/Asset/Material/MaterialAsset.hpp>
#include <Tsukino/Engine/Asset/Material/MaterialAssetBuilder.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Util/AssetHandleGenerator.hpp>

#include <Tsukino/Core/Log.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>

#include <cstdint>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する関数
    //--------------------------------------------------------------
    bool ModelLoader::CanLoad(const std::string& ext) const {
        return ext == ".tsm";
    }

    //--------------------------------------------------------------
    //! @brief モデルファイルを読み込み ModelAsset を生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> ModelLoader::Load(const Tsukino::Core::Path& path) {
        const std::string& filePath = path.string();

        const std::vector<std::uint8_t> bytes = Tsukino::IO::FileSystem::ReadBinary(path);
        if(bytes.empty()) {
            Tsukino::Core::Log::Error("ModelLoader: Failed to open model (missing or empty): " + filePath);
            return nullptr;
        }
        std::istringstream ifs(std::string(bytes.begin(), bytes.end()), std::ios::binary);

        Tsukino::Core::Ref<ModelAsset> asset = Tsukino::Core::CreateRef<ModelAsset>();

        try {
            cereal::BinaryInputArchive archive(ifs);
            archive(asset->modelData);
        } catch(const std::exception& e) {
            Tsukino::Core::Log::Error("ModelLoader: Failed to deserialize .tsm: " + filePath + " - " + e.what());
            return nullptr;
        }

        //--------------------------------------------------------------
        // マテリアルのテクスチャをロードしてMaterialAssetを構築
        //--------------------------------------------------------------
        if(m_assetManager) {
            asset->materialHandles.resize(asset->modelData.materials.size());

            for(u32 i = 0; i < asset->modelData.materials.size(); ++i) {
                const auto& matData = asset->modelData.materials[i];

                // テクスチャを読み込んでマテリアルアセットを組み立てる（単体のマテリアルファイル .tmat と同じ手順）
                auto matAsset = BuildMaterialAsset(*m_assetManager, matData);

                // MaterialAssetをAssetManagerに登録してハンドルを取得。
                // マテリアルは単体のファイルを持たないため、モデルのパスと
                // マテリアル番号を合わせたものを識別キーにする
                AssetHandle matHandle = AssetHandleGenerator::GenerateFromKey(filePath + "|material|" + std::to_string(i));
                matAsset->SetHandle(matHandle);
                m_assetManager->RegisterAsset(matHandle, matAsset);

                asset->materialHandles[i] = matHandle;
            }
        }

        Tsukino::Core::Log::Info("ModelLoader: Successfully loaded .tsm (" + std::to_string(asset->modelData.nodes.size()) + " mesh nodes): " + filePath);

        return asset;
    }
}    // namespace Tsukino::Asset

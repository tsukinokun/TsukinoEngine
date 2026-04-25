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

        Tsukino::Core::Ref<ModelAsset> asset = Tsukino::Core::CreateRef<ModelAsset>();

        try {
            cereal::BinaryInputArchive archive(ifs);
            archive(asset->modelData);
        } catch (const std::exception& e) {
            Tsukino::Core::Log::Error("ModelLoader: Failed to deserialize .tsm: " + filePath + " - " + e.what());
            return nullptr;
        }

        Tsukino::Core::Log::Info("ModelLoader: Successfully loaded .tsm (" + std::to_string(asset->modelData.nodes.size()) + " mesh nodes): " + filePath);
        return asset;
    }
}    // namespace Tsukino::Asset

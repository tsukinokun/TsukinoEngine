//----------------------------------------------------------------------------
//! @file   MaterialLoader.cpp
//! @brief  マテリアルファイル（.tmat）のキャッシュを読み込むローダーの実装
//----------------------------------------------------------------------------
#include <Tsukino/Engine/Asset/Material/MaterialLoader.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Material/MaterialAsset.hpp>
#include <Tsukino/Engine/Asset/Material/MaterialAssetBuilder.hpp>
#include <Tsukino/Core/Log.hpp>
// MaterialData の hlslpp::interop のシリアライズはここで定義されている
#include <Tsukino/GraphicsCommon/Model/ModelData.hpp>

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include <fstream>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {

    //----------------------------------------------------------------------------
    //! 対応する拡張子か判定します。
    //----------------------------------------------------------------------------
    bool MaterialLoader::CanLoad(const std::string& ext) const {
        return ext == ".tmat";
    }

    //----------------------------------------------------------------------------
    //! .tmat のキャッシュを読み込み、MaterialAsset を生成します。
    //----------------------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> MaterialLoader::Load(const Tsukino::Core::Path& path) {
        if(!m_assetManager)
            return nullptr;

        std::ifstream file(path.string(), std::ios::binary);
        if(!file.is_open()) {
            Tsukino::Core::Log::Error("MaterialLoader: Failed to open " + path.string());
            return nullptr;
        }

        Tsukino::GraphicsCommon::MaterialData data;
        try {
            cereal::BinaryInputArchive archive(file);
            archive(data);
        } catch(const std::exception& e) {
            Tsukino::Core::Log::Error("MaterialLoader: Failed to deserialize " + path.string() + " - " + e.what());
            return nullptr;
        }

        // テクスチャを読み込んでマテリアルアセットを組み立てる（モデルの中のマテリアルと同じ手順）
        return BuildMaterialAsset(*m_assetManager, data);
    }
}    // namespace Tsukino::Asset

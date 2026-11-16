//--------------------------------------------------------------
//! @file   CubemapLoader.cpp
//! @brief  キューブマップアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Cubemap/CubemapLoader.hpp>
#include <Tsukino/Engine/Asset/Cubemap/CubemapAsset.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>

#include <vector>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する関数
    //--------------------------------------------------------------
    bool CubemapLoader::CanLoad(const std::string& ext) const {
        return ext == ".tcc";
    }

    //--------------------------------------------------------------
    //! @brief キューブマップファイルを読み込み CubemapAsset を生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> CubemapLoader::Load(const Tsukino::Core::Path& path) {
        std::vector<uint8_t> buffer = Tsukino::IO::FileSystem::ReadBinary(path);
        if(buffer.empty()) {
            Tsukino::Core::Log::Error("CubemapLoader: Failed to open (missing or empty): " + path.string());
            return nullptr;
        }

        Tsukino::Core::Ref<CubemapAsset> asset = Tsukino::Core::CreateRef<CubemapAsset>();
        asset->data                            = std::move(buffer);

        Tsukino::Core::Log::Info("CubemapLoader: Successfully loaded: " + path.string());
        return asset;
    }

}    // namespace Tsukino::Asset

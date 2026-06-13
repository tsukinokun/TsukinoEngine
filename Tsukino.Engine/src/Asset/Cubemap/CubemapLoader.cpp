//--------------------------------------------------------------
//! @file   CubemapLoader.cpp
//! @brief  キューブマップアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Cubemap/CubemapLoader.hpp>
#include <Tsukino/Engine/Asset/Cubemap/CubemapAsset.hpp>

#include <Tsukino/Core/Log.hpp>

#include <fstream>
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
        //--------------------------------------------------------------
        // バイナリファイルを開く
        //--------------------------------------------------------------
        std::ifstream file(path.string(), std::ios::binary);
        if(!file.is_open()) {
            Tsukino::Core::Log::Error("CubemapLoader: Failed to open: " + path.string());
            return nullptr;
        }

        //--------------------------------------------------------------
        // 空ファイルチェック
        //--------------------------------------------------------------
        file.seekg(0, std::ios::end);
        const auto size = file.tellg();
        if(size <= 0) {
            Tsukino::Core::Log::Error("CubemapLoader: Invalid file (empty): " + path.string());
            return nullptr;
        }

        //--------------------------------------------------------------
        // バイナリデータをバッファに格納
        //--------------------------------------------------------------
        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> buffer(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(buffer.data()), size);

        //--------------------------------------------------------------
        // CubemapAsset 生成
        //--------------------------------------------------------------
        Tsukino::Core::Ref<CubemapAsset> asset = Tsukino::Core::CreateRef<CubemapAsset>();
        asset->ddsData                         = std::move(buffer);

        Tsukino::Core::Log::Info("CubemapLoader: Successfully loaded: " + path.string());
        return asset;
    }

}    // namespace Tsukino::Asset

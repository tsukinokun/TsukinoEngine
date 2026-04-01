//--------------------------------------------------------------
//! @file   FontLoader.cpp
//! @brief  フォントアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Font/FontLoader.hpp>
#include <Tsukino/Engine/Asset/Font/FontAsset.hpp>

#include <Tsukino/Core/Log.hpp>

#include <fstream>
#include <vector>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する関数
    //--------------------------------------------------------------
    bool FontLoader::CanLoad(const std::string& ext) const {
        return ext == ".spritefont";
    }

    //--------------------------------------------------------------
    //! @brief フォントファイルを読み込み FontAsset を生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> FontLoader::Load(const Tsukino::Core::Path& path) {
        //--------------------------------------------------------------
        // バイナリファイルを開く
        //--------------------------------------------------------------
        std::ifstream file(path.string(), std::ios::binary);
        if(!file.is_open()) {
            Tsukino::Core::Log::Error("Failed to open spritefont: " + path.string());
            return nullptr;
        }

        //--------------------------------------------------------------
        // 空ファイルチェック
        //--------------------------------------------------------------
        file.seekg(0, std::ios::end);
        const auto size = file.tellg();
        if(size <= 0) {
            Tsukino::Core::Log::Error("Invalid spritefont (empty): " + path.string());
            return nullptr;
        }

        //--------------------------------------------------------------
        // バイナリデータをバッファに格納
        //--------------------------------------------------------------
        std::vector<uint8_t> buffer(size);
        file.read(reinterpret_cast<char*>(buffer.data()), size);

        //--------------------------------------------------------------
        // FontAsset 生成
        //--------------------------------------------------------------
        Tsukino::Core::Ref<FontAsset> asset = Tsukino::Core::CreateRef<FontAsset>();

        //--------------------------------------------------------------
        // バイナリデータを格納
        //--------------------------------------------------------------
        asset->m_binaryData = std::move(buffer);

        // FontAssetを返す
        return asset;
    }

}    // namespace Tsukino::Asset

//--------------------------------------------------------------
//! @file   DynamicFontLoader.cpp
//! @brief  動的フォントアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Font/DynamicFontLoader.hpp>
#include <Tsukino/Engine/Asset/Font/DynamicFontAsset.hpp>

#include <Tsukino/Core/Log.hpp>

#include <fstream>
#include <vector>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する関数
    //--------------------------------------------------------------
    bool DynamicFontLoader::CanLoad(const std::string& ext) const {
        return ext == ".dfont";
    }

    //--------------------------------------------------------------
    //! @brief .dfont キャッシュファイルを読み込み DynamicFontAsset を生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> DynamicFontLoader::Load(const Tsukino::Core::Path& path) {
        std::ifstream file(path.string(), std::ios::binary);
        if(!file.is_open()) {
            Tsukino::Core::Log::Error("Failed to open dfont: " + path.string());
            return nullptr;
        }

        //--------------------------------------------------------------
        // フォーマット: [u32 faceNameLen][faceName utf-8 bytes][float pixelSize][u32 fontDataLen][fontData bytes]
        //--------------------------------------------------------------
        uint32_t faceNameLen = 0;
        if(!file.read(reinterpret_cast<char*>(&faceNameLen), sizeof(faceNameLen))) {
            Tsukino::Core::Log::Error("Invalid dfont (header): " + path.string());
            return nullptr;
        }

        std::string faceName(faceNameLen, '\0');
        if(faceNameLen > 0 && !file.read(faceName.data(), faceNameLen)) {
            Tsukino::Core::Log::Error("Invalid dfont (faceName): " + path.string());
            return nullptr;
        }

        float pixelSize = 0.0f;
        if(!file.read(reinterpret_cast<char*>(&pixelSize), sizeof(pixelSize))) {
            Tsukino::Core::Log::Error("Invalid dfont (pixelSize): " + path.string());
            return nullptr;
        }

        uint32_t fontDataLen = 0;
        if(!file.read(reinterpret_cast<char*>(&fontDataLen), sizeof(fontDataLen))) {
            Tsukino::Core::Log::Error("Invalid dfont (fontDataLen): " + path.string());
            return nullptr;
        }

        std::vector<uint8_t> fontData(fontDataLen);
        if(fontDataLen > 0 && !file.read(reinterpret_cast<char*>(fontData.data()), fontDataLen)) {
            Tsukino::Core::Log::Error("Invalid dfont (fontData): " + path.string());
            return nullptr;
        }

        //--------------------------------------------------------------
        // DynamicFontAsset 生成
        //--------------------------------------------------------------
        Tsukino::Core::Ref<DynamicFontAsset> asset = Tsukino::Core::CreateRef<DynamicFontAsset>();
        asset->m_faceName                          = std::move(faceName);
        asset->m_pixelSize                         = pixelSize;
        asset->m_fontFileData                      = std::move(fontData);

        return asset;
    }

}    // namespace Tsukino::Asset

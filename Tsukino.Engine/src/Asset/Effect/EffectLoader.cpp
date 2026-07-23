//--------------------------------------------------------------
//! @file   EffectLoader.cpp
//! @brief  エフェクトアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Effect/EffectLoader.hpp>
#include <Tsukino/Engine/Asset/Effect/EffectAsset.hpp>

#include <fstream>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する関数
    //--------------------------------------------------------------
    bool EffectLoader::CanLoad(const std::string& ext) const {
        return ext == ".efk";
    }

    //--------------------------------------------------------------
    //! @brief .efk ファイルを読み込み EffectAsset を生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> EffectLoader::Load(const Tsukino::Core::Path& path) {
        std::ifstream file(path.string(), std::ios::binary);
        if(!file.is_open()) {
            return nullptr;
        }

        file.seekg(0, std::ios::end);
        const std::streamsize size = file.tellg();
        if(size <= 0) {
            return nullptr;
        }

        file.seekg(0, std::ios::beg);
        std::vector<u8> buffer(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(buffer.data()), size);

        Tsukino::Core::Ref<EffectAsset> asset = Tsukino::Core::CreateRef<EffectAsset>();
        asset->binary = std::move(buffer);

        return asset;
    }

}    // namespace Tsukino::Asset

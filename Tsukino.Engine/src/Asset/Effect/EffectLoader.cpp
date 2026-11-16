//--------------------------------------------------------------
//! @file   EffectLoader.cpp
//! @brief  エフェクトアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Effect/EffectLoader.hpp>
#include <Tsukino/Engine/Asset/Effect/EffectAsset.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>

#include <vector>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する関数
    //--------------------------------------------------------------
    bool EffectLoader::CanLoad(const std::string& ext) const {
        return ext == ".efk" || ext == ".efkefc";
    }

    //--------------------------------------------------------------
    //! @brief .efk ファイルを読み込み EffectAsset を生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> EffectLoader::Load(const Tsukino::Core::Path& path) {
        std::vector<u8> buffer = Tsukino::IO::FileSystem::ReadBinary(path);
        if(buffer.empty()) {
            return nullptr;
        }

        Tsukino::Core::Ref<EffectAsset> asset = Tsukino::Core::CreateRef<EffectAsset>();
        asset->binary                         = std::move(buffer);

        return asset;
    }

}    // namespace Tsukino::Asset

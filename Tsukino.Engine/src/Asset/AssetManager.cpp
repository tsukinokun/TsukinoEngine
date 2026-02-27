//--------------------------------------------------------------
//! @file   AssetManager.cpp
//! @brief  アセット管理クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/AssetMap.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    // staticメンバ変数の実体
    AssetMap                       AssetManager::s_Assets;
    std::vector<Tsukino::Core::Ref<IAssetLoader>> AssetManager::s_Loaders;

    //--------------------------------------------------------------
    //! @brief アセットハンドルが存在するか確認する関数
    //--------------------------------------------------------------
    bool AssetManager::Exists(AssetHandle handle) {
        return s_Assets.contains(handle.Value());
    }

    //--------------------------------------------------------------
    //! @brief 任意のアセットハンドルからアセットを取得する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> AssetManager::Get(AssetHandle handle) {
        auto it = s_Assets.find(handle.Value());
        return it != s_Assets.end() ? it->second : nullptr;
    }

    //--------------------------------------------------------------
    //! @brief ローダーを登録する関数
    //--------------------------------------------------------------
    void AssetManager::RegisterLoader(Tsukino::Core::Ref<IAssetLoader> loader) {
        s_Loaders.push_back(loader);
    }
}    // namespace Tsukino::Asset

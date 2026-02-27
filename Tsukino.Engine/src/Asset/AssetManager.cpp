//--------------------------------------------------------------
//! @file   AssetManager.cpp
//! @brief  アセット管理クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/IAsset.hpp>
#include <Tsukino/Engine/Asset/IAssetLoader.hpp>
#include <Tsukino/Engine/Asset/Util/AssetHandleGenerator.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    // staticメンバ変数の実体
    AssetMap                                      AssetManager::s_Assets;
    std::vector<Tsukino::Core::Ref<IAssetLoader>> AssetManager::s_Loaders;

    //--------------------------------------------------------------
    //! @brief アセットをロードする関数
    //--------------------------------------------------------------
    AssetHandle AssetManager::Load(const Tsukino::Core::Path& path) {
        //--------------------------------------------------------------
        // 拡張子取得
        //--------------------------------------------------------------
        std::string ext = path.extension();                                // 拡張子を取得
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);    //拡張子を小文字化

        //--------------------------------------------------------------
        // ローダー探索
        //--------------------------------------------------------------
        Tsukino::Core::Ref<IAssetLoader> selected = nullptr;    // 対応するローダーを格納する変数
        for(auto& loader : s_Loaders) {
            if(loader->CanLoad(ext)) {
                selected = loader;
                break;
            }
        }

        if(!selected) {
            // ローダーが見つからない
            return AssetHandle::Invalid();    // 無効なハンドルを返す
        }

        //--------------------------------------------------------------
        // ロード実行
        //--------------------------------------------------------------
        Tsukino::Core::Ref<IAsset> asset = selected->Load(path);
        if(!asset) {
            return AssetHandle::Invalid();    // 無効なハンドルを返す
        }
        //--------------------------------------------------------------
        // AssetMapに登録
        //--------------------------------------------------------------
        AssetHandle handle = AssetHandleGenerator::Generate();    // ハンドルを生成
        s_Assets.insert({handle.Value(), asset});                 // ハンドルとアセットをAssetMapに登録

        //--------------------------------------------------------------
        // ハンドルを返す
        //--------------------------------------------------------------
        return handle;
    }

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

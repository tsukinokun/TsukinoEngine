//--------------------------------------------------------------
//! @file   AssetManager.cpp
//! @brief  アセット管理クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/IAsset.hpp>
#include <Tsukino/Engine/Asset/IAssetLoader.hpp>
#include <Tsukino/Engine/Asset/Util/AssetHandleGenerator.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderLoader.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureImporter.hpp>

#include <Tsukino/Core/Log.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    AssetMap                                                          AssetManager::s_Assets;       // アセットマップの定義
    std::vector<Tsukino::Core::Ref<IAssetLoader>>                     AssetManager::s_Loaders;      // ローダーリストの定義
    std::unordered_map<AssetType, Tsukino::Core::Ref<IAssetImporter>> AssetManager::s_Importers;    // インポーターの定義

    //--------------------------------------------------------------
    //! @brief AssetManagerを初期化する関数
    //--------------------------------------------------------------
    void AssetManager::Initialize() {
        //--------------------------------------------------------------
        // ローダー登録
        //--------------------------------------------------------------
        RegisterLoader(Tsukino::Core::CreateRef<ShaderLoader>());    // シェーダローダーを登録

        //--------------------------------------------------------------
        // インポーター登録
        //--------------------------------------------------------------
        RegisterImporter(AssetType::Texture, Tsukino::Core::CreateRef<TextureImporter>());    // テクスチャインポーターを登録
    }

    //--------------------------------------------------------------
    //! @brief AssetManagerを後処理する関数
    //--------------------------------------------------------------
    void AssetManager::Destroy() {
        s_Assets.clear();     // AssetMapをクリア
        s_Loaders.clear();    // ローダーリストをクリア
    }

    //--------------------------------------------------------------
    //! @brief アセットをロードする関数
    //--------------------------------------------------------------
    AssetHandle AssetManager::Load(const Tsukino::Core::Path& path) {
        std::string ext = path.extension();                                // 拡張子を取得
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);    // 拡張子を小文字に変換

        AssetType type = GetAssetTypeFromExtension(ext);    // 拡張子からアセットの種類を取得

        //------------------------------------------------
        // Importer
        //------------------------------------------------

        auto importerIt = s_Importers.find(type);    // 対応するインポーターを検索

        // インポーターが見つかった場合はインポート処理を実行
        if(importerIt != s_Importers.end()) {
            Tsukino::Core::Path cacheDir("Cache/");    // キャッシュディレクトリのパスを指定

            importerIt->second->Import(path, cacheDir);
        }

        //------------------------------------------------
        // Loader
        //------------------------------------------------

        Tsukino::Core::Path loadPath = ConvertToCachePath(path);

        for(auto& loader : s_Loaders) {
            if(loader->CanLoad(loadPath.extension())) {
                Tsukino::Core::Ref<IAsset> asset = loader->Load(loadPath);

                AssetHandle handle = AssetHandleGenerator::Generate();
                s_Assets.insert({handle.Value(), asset});

                return handle;
            }
        }

        return AssetHandle::Invalid();
    }

    //--------------------------------------------------------------
    //! @brief 任意のアセットハンドルからアセットを取得する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> AssetManager::Get(AssetHandle handle) {
        auto it = s_Assets.find(handle.Value());
        return it != s_Assets.end() ? it->second : nullptr;
    }

    //--------------------------------------------------------------
    //! @brief アセットハンドルが存在するか確認する関数
    //--------------------------------------------------------------
    bool AssetManager::Exists(AssetHandle handle) {
        return s_Assets.contains(handle.Value());
    }

    //--------------------------------------------------------------
    //! @brief インポーターを登録する関数
    //--------------------------------------------------------------
    void AssetManager::RegisterImporter(AssetType type, Tsukino::Core::Ref<IAssetImporter> importer) {
        // インポーターが null でないことを確認
        if(!importer) {
            Tsukino::Core::Log::Error(std::format("AssetManager::RegisterImporter - Importer is null for AssetType {}", (int)type));
            return;
        }

        // すでに同じAssetTypeのインポーターが登録されている場合は警告を出す
        if(s_Importers.contains(type)) {
            Tsukino::Core::Log::Warn(std::format("AssetManager::RegisterImporter - Importer for AssetType {} is already registered. Overwriting.", (int)type));
        }

        // インポーターを登録
        s_Importers[type] = importer;

        // ログ出力
        Tsukino::Core::Log::Info(std::format("Registered importer for AssetType {}", (int)type));
    }

    //--------------------------------------------------------------
    //! @brief ローダーを登録する関数
    //--------------------------------------------------------------
    void AssetManager::RegisterLoader(Tsukino::Core::Ref<IAssetLoader> loader) {
        s_Loaders.push_back(loader);
    }

    //--------------------------------------------------------------
    //! @brief  拡張子からアセットの種類を取得する関数
    //--------------------------------------------------------------
    AssetType AssetManager::GetAssetTypeFromExtension(const std::string& ext) {
        // 拡張子とアセットの種類のテーブル
        static const std::unordered_map<std::string, AssetType> extensionToAssetType = {
            // Texture
            {".png",    AssetType::Texture},
            {".jpg",    AssetType::Texture},
            {".jpeg",   AssetType::Texture},
            {".bmp",    AssetType::Texture},
            {".tga",    AssetType::Texture},
            {".dds",    AssetType::Texture},

            // Shader
            {".shader", AssetType::Shader },
            {".hlsl",   AssetType::Shader },

            // Mesh
            {".obj",    AssetType::Mesh   },
            {".fbx",    AssetType::Mesh   },
            {".gltf",   AssetType::Mesh   },
            {".glb",    AssetType::Mesh   },

            // Audio
            {".wav",    AssetType::Audio  },
            {".mp3",    AssetType::Audio  },
            {".ogg",    AssetType::Audio  },
            {".flac",   AssetType::Audio  },
        };

        // テーブルから拡張子に対応するアセットの種類を取得
        if(auto it = extensionToAssetType.find(ext); it != extensionToAssetType.end())
            return it->second;

        // 対応する種類がない場合はNoneを返す
        return AssetType::None;
    }

    //--------------------------------------------------------------
    //! @brief  ソースパスからキャッシュパスに変換する関数
    //--------------------------------------------------------------
    Tsukino::Core::Path AssetManager::ConvertToCachePath(const Tsukino::Core::Path& sourcePath) {
        std::string ext = sourcePath.extension();                          // 拡張子を取得
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);    // 拡張子を小文字に変換

        std::string name = sourcePath.stem();    // 拡張子を除いたファイル名を取得

        Tsukino::Core::Path cacheDir("Cache");    // キャッシュディレクトリのパスを指定

        //--------------------------------------------------------------
        // テクスチャ
        //--------------------------------------------------------------
        if(ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga") {
            return cacheDir / (name + ".dds");
        }

        //--------------------------------------------------------------
        // シェーダー
        //--------------------------------------------------------------
        if(ext == ".hlsl" || ext == ".shader") {
            return cacheDir / (name + ".cso");
        }

        // 対応する変換ルールがない場合は元のパスを返す
        return sourcePath;
    }

}    // namespace Tsukino::Asset

//--------------------------------------------------------------
//! @file   AssetManager.cpp
//! @brief  アセット管理クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/IAsset.hpp>
#include <Tsukino/Engine/Asset/IAssetLoader.hpp>
#include <Tsukino/Engine/Asset/Util/AssetHandleGenerator.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureLoder.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderLoader.hpp>
#include <Tsukino/Engine/Asset/Font/FontLoader.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureImporter.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderImporter.hpp>
#include <Tsukino/Engine/Asset/Font/FontImporter.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief デストラクタ
    //--------------------------------------------------------------
    AssetManager::~AssetManager() {
        s_Assets.clear();     // AssetMapをクリア
        s_Loaders.clear();    // ローダーリストをクリア
    }

    //--------------------------------------------------------------
    //! @brief AssetManagerを初期化する関数
    //--------------------------------------------------------------
    void AssetManager::Initialize() {
        //--------------------------------------------------------------
        // ローダー登録
        //--------------------------------------------------------------
        RegisterLoader(Tsukino::Core::CreateRef<ShaderLoader>());     // シェーダローダーを登録
        RegisterLoader(Tsukino::Core::CreateRef<TextureLoader>());    // テクスチャローダーを登録
        RegisterLoader(Tsukino::Core::CreateRef<FontLoader>());       // フォントローダーを登録

        //--------------------------------------------------------------
        // インポーター登録
        //--------------------------------------------------------------
        RegisterImporter(AssetType::Shader, Tsukino::Core::CreateRef<ShaderImporter>());      // シェーダーインポーターの登録
        RegisterImporter(AssetType::Texture, Tsukino::Core::CreateRef<TextureImporter>());    // テクスチャインポーターを登録
        RegisterImporter(AssetType::Font, Tsukino::Core::CreateRef<FontImporter>());          // フォントインポーターの登録
    }

    //--------------------------------------------------------------
    //! @brief アセットをロードする関数
    //--------------------------------------------------------------
    AssetHandle AssetManager::Load(const Tsukino::Core::Path& path) {
        std::string ext = path.extension();                                // 拡張子を取得
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);    // 拡張子を小文字に変換

        AssetType           type      = GetAssetTypeFromExtension(ext);    // 拡張子からアセットの種類を取得
        Tsukino::Core::Path cachePath = ConvertToCachePath(path);          // ソースパスからキャッシュパスに変換

        //------------------------------------------------
        // Importer
        //------------------------------------------------

        auto importerIt = s_Importers.find(type);    // 対応するインポーターを検索

        // インポーターが見つかった場合はインポート処理を実行
        if(importerIt != s_Importers.end()) {
            bool shouldImport = false;    // インポートが必要かどうかのフラグ

            //------------------------------------------------
            // キャッシュが存在しない
            //------------------------------------------------
            if(!Tsukino::IO::FileSystem::Exists(cachePath)) {
                shouldImport = true;
            } else {
                //------------------------------------------------
                // ソースの方が新しいかチェック
                //------------------------------------------------
                Tsukino::Core::Path             baseDir    = Tsukino::IO::FileSystem::GetAssetRootPath();
                std::filesystem::file_time_type sourceTime = Tsukino::IO::FileSystem::GetLastWriteTime(baseDir / path);
                std::filesystem::file_time_type cacheTime  = Tsukino::IO::FileSystem::GetLastWriteTime(cachePath);

                if(sourceTime > cacheTime) {
                    Tsukino::Core::Log::Info("Asset updated. Re-importing: " + path.string());
                    shouldImport = true;
                }
            }

            //------------------------------------------------
            // キャッシュが存在しない、またはソースの方が新しい場合はインポートを実行
            //------------------------------------------------
            if(shouldImport) {
                Tsukino::Core::Path cacheDir = Tsukino::IO::FileSystem::GetAssetRootPath() / "Cache";
                // インポーターに相対パス(path)を渡すことで、インポーター側で階層を作ってもらう
                importerIt->second->Import(path, cacheDir);
            }
        }

        //------------------------------------------------
        // Loader
        //------------------------------------------------
        // ローダーリストを順番にチェックして、対応するローダーでロードを試みる
        for(auto& loader : s_Loaders) {
            if(loader->CanLoad(cachePath.extension())) {
                Tsukino::Core::Ref<IAsset> asset = loader->Load(cachePath);
                if(!asset)
                    continue;

                // アセットが正常にロードできた場合はハンドルを生成してマップに登録し、ハンドルを返す
                AssetHandle handle = AssetHandleGenerator::Generate();
                s_Assets.insert({handle.Value(), asset});
                return handle;
            }
        }

        // 対応するローダーが見つからない、またはロードに失敗した場合はエラーログを出力して無効なハンドルを返す
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

            // Font
            {".ttf",    AssetType::Font   },
            {".otf",    AssetType::Font   },
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
        //--------------------------------------------------------------
        // ソース拡張子とキャッシュ拡張子のマッピング
        //--------------------------------------------------------------
        static const std::unordered_map<std::string, std::string> extensionMap = {
            // シェーダー
            {".hlsl",   ".cso"       },
            {".shader", ".cso"       },

            // テクスチャ
            {".png",    ".dds"       },
            {".jpg",    ".dds"       },
            {".jpeg",   ".dds"       },
            {".tga",    ".dds"       },
            {".bmp",    ".dds"       },

            // フォント
            {".ttf",    ".spritefont"},
            {".otf",    ".spritefont"}
        };

        //--------------------------------------------------------------
        // Cacheディレクトリを基点にする
        //--------------------------------------------------------------
        Tsukino::Core::Path cachePath = Tsukino::IO::FileSystem::GetAssetRootPath() / "Cache" / sourcePath;

        //--------------------------------------------------------------
        // 拡張子を小文字に変換して判定
        //--------------------------------------------------------------
        std::string ext = sourcePath.extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        //--------------------------------------------------------------
        // 拡張子をキャッシュ用に置換
        //--------------------------------------------------------------
        // マップにあれば置換
        if(auto it = extensionMap.find(ext); it != extensionMap.end()) {
            cachePath.replace_extension(it->second);
        }

        return cachePath;
    }

}    // namespace Tsukino::Asset

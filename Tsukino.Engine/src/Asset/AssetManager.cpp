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
#include <Tsukino/Engine/Asset/Audio/AudioLoader.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureImporter.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderImporter.hpp>
#include <Tsukino/Engine/Asset/Font/FontImporter.hpp>
#include <Tsukino/Engine/Asset/Audio/AudioImporter.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>
// 名前空間 : Tsukino::Asset
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
        RegisterLoader(Tsukino::Core::CreateRef<AudioLoader>());      // オーディオローダーを登録

        //--------------------------------------------------------------
        // インポーター登録
        //--------------------------------------------------------------
        RegisterImporter(AssetType::Shader, Tsukino::Core::CreateRef<ShaderImporter>());      // シェーダーインポーターの登録
        RegisterImporter(AssetType::Texture, Tsukino::Core::CreateRef<TextureImporter>());    // テクスチャインポーターを登録
        RegisterImporter(AssetType::Font, Tsukino::Core::CreateRef<FontImporter>());          // フォントインポーターの登録
        RegisterImporter(AssetType::Audio, Tsukino::Core::CreateRef<AudioImporter>());        // オーディオインポーターの登録
    }

    //--------------------------------------------------------------
    //! @brief アセットをロードする関数
    //--------------------------------------------------------------
    AssetHandle AssetManager::Load(const Tsukino::Core::Path& path) {
        //--------------------------------------------------------------
        // 入力パスを basePath と fragment に分解
        //--------------------------------------------------------------
        auto [sourceBase, sourceFragment] = Tsukino::Core::Path::SplitPathAndFragment(path.string());
        Tsukino::Core::Path sourceBasePath(sourceBase);

        std::string ext  = Tsukino::Core::Path::ToLower(sourceBasePath.extension());    // 拡張子は必ずベースパスで判定
        AssetType   type = GetAssetTypeFromExtension(ext);

        // ConvertToCachePath は fragment を保持して返す
        Tsukino::Core::Path cachePathWithFragment = ConvertToCachePath(path);

        // ファイルI/Oは fragment なしのキャッシュパスで行う
        auto [cacheBase, cacheFragment] = Tsukino::Core::Path::SplitPathAndFragment(cachePathWithFragment.string());
        Tsukino::Core::Path cacheBasePath(cacheBase);

        //------------------------------------------------
        // Importer
        //------------------------------------------------
        auto importerIt = s_Importers.find(type);    // 対応するインポーターを検索
        if(importerIt != s_Importers.end()) {
            bool shouldImport = false;    // インポートが必要かどうかのフラグ

            if(!Tsukino::IO::FileSystem::Exists(cacheBasePath)) {
                shouldImport = true;
            } else {
                Tsukino::Core::Path             baseDir    = Tsukino::IO::FileSystem::GetAssetRootPath();
                std::filesystem::file_time_type sourceTime = Tsukino::IO::FileSystem::GetLastWriteTime(baseDir / sourceBasePath);
                std::filesystem::file_time_type cacheTime  = Tsukino::IO::FileSystem::GetLastWriteTime(cacheBasePath);

                if(sourceTime > cacheTime) {
                    Tsukino::Core::Log::Info("Asset updated. Re-importing: " + path.string());
                    shouldImport = true;
                }
            }

            if(shouldImport) {
                Tsukino::Core::Path cacheDir = Tsukino::IO::FileSystem::GetAssetRootPath() / "Cache";
                importerIt->second->Import(sourceBasePath, cacheDir);
            }
        }

        //------------------------------------------------
        // Loader
        //------------------------------------------------
        for(auto& loader : s_Loaders) {
            if(loader->CanLoad(cacheBasePath.extension())) {
                // ローダーには fragment 付きで渡す（Audio サブリソース解決用）
                Tsukino::Core::Ref<IAsset> asset = loader->Load(cachePathWithFragment);
                if(!asset)
                    continue;

                AssetHandle handle = AssetHandleGenerator::Generate();
                asset->SetHandle(handle);
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
        if(!importer) {
            Tsukino::Core::Log::Error(std::format("AssetManager::RegisterImporter - Importer is null for AssetType {}", (int)type));
            return;
        }

        if(s_Importers.contains(type)) {
            Tsukino::Core::Log::Warn(std::format("AssetManager::RegisterImporter - Importer for AssetType {} is already registered. Overwriting.", (int)type));
        }

        s_Importers[type] = importer;
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
        static const std::unordered_map<std::string, AssetType> extensionToAssetType = {
            {".png",    AssetType::Texture},
            {".jpg",    AssetType::Texture},
            {".jpeg",   AssetType::Texture},
            {".bmp",    AssetType::Texture},
            {".tga",    AssetType::Texture},
            {".dds",    AssetType::Texture},

            {".shader", AssetType::Shader },
            {".hlsl",   AssetType::Shader },

            {".obj",    AssetType::Mesh   },
            {".fbx",    AssetType::Mesh   },
            {".gltf",   AssetType::Mesh   },
            {".glb",    AssetType::Mesh   },

            {".wav",    AssetType::Audio  },

            {".font",   AssetType::Font   },
        };

        if(auto it = extensionToAssetType.find(ext); it != extensionToAssetType.end())
            return it->second;

        return AssetType::None;
    }

    //--------------------------------------------------------------
    //! @brief  ソースパスからキャッシュパスに変換する関数
    //--------------------------------------------------------------
    Tsukino::Core::Path AssetManager::ConvertToCachePath(const Tsukino::Core::Path& sourcePath) {
        static const std::unordered_map<std::string, std::string> extensionMap = {
            {".hlsl",   ".cso"       },
            {".shader", ".cso"       },

            {".png",    ".dds"       },
            {".jpg",    ".dds"       },
            {".jpeg",   ".dds"       },
            {".tga",    ".dds"       },
            {".bmp",    ".dds"       },

            {".font",   ".spritefont"},

            // Audio
            {".wav",    ".xwb"       },
        };

        auto [sourceBase, sourceFragment] = Tsukino::Core::Path::SplitPathAndFragment(sourcePath.string());
        Tsukino::Core::Path sourceBasePath(sourceBase);

        Tsukino::Core::Path cacheBasePath = Tsukino::IO::FileSystem::GetAssetRootPath() / "Cache" / sourceBasePath;

        std::string ext = Tsukino::Core::Path::ToLower(sourceBasePath.extension());
        if(auto it = extensionMap.find(ext); it != extensionMap.end()) {
            cacheBasePath.replace_extension(it->second);
        }

        if(sourceFragment.empty()) {
            return cacheBasePath;
        }

        return Tsukino::Core::Path(cacheBasePath.string() + "#" + sourceFragment);
    }

}    // namespace Tsukino::Asset

//--------------------------------------------------------------
//! @file   AssetManager.cpp
//! @brief  アセット管理クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/IAsset.hpp>
#include <Tsukino/Engine/Asset/IAssetLoader.hpp>
#include <Tsukino/Engine/Asset/Util/AssetHandleGenerator.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureLoader.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderLoader.hpp>
#include <Tsukino/Engine/Asset/Font/FontLoader.hpp>
#include <Tsukino/Engine/Asset/Font/DynamicFontLoader.hpp>
#include <Tsukino/Engine/Asset/Audio/AudioLoader.hpp>
#include <Tsukino/Engine/Asset/Model/ModelLoader.hpp>
#include <Tsukino/Engine/Asset/Cubemap/CubemapLoader.hpp>
#include <Tsukino/Engine/Asset/Effect/EffectLoader.hpp>
#include <Tsukino/Engine/Asset/Effect/EffectImporter.hpp>

#include <Tsukino/Engine/Asset/Texture/TextureImporter.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderImporter.hpp>
#include <Tsukino/Engine/Asset/Font/FontImporter.hpp>
#include <Tsukino/Engine/Asset/Font/DynamicFontImporter.hpp>
#include <Tsukino/Engine/Asset/Audio/AudioImporter.hpp>
#include <Tsukino/Engine/Asset/Model/ModelImporter.hpp>
#include <Tsukino/Engine/Asset/Cubemap/CubemapImporter.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>

#include <filesystem>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief デストラクタ
    //--------------------------------------------------------------
    AssetManager::~AssetManager() {
        m_assets.clear();     // AssetMapをクリア
        m_loaders.clear();    // ローダーリストをクリア
    }

    //--------------------------------------------------------------
    //! @brief AssetManagerを初期化する関数
    //--------------------------------------------------------------
    void AssetManager::Initialize() {
        //--------------------------------------------------------------
        // ローダー登録
        //--------------------------------------------------------------
        RegisterLoader(Tsukino::Core::CreateRef<ShaderLoader>());       // シェーダローダーを登録
        RegisterLoader(Tsukino::Core::CreateRef<TextureLoader>());      // テクスチャローダーを登録
        RegisterLoader(Tsukino::Core::CreateRef<FontLoader>());         // フォントローダーを登録
        RegisterLoader(Tsukino::Core::CreateRef<DynamicFontLoader>());  // 動的フォントローダーを登録
        RegisterLoader(Tsukino::Core::CreateRef<AudioLoader>());        // オーディオローダーを登録
        RegisterLoader(Tsukino::Core::CreateRef<ModelLoader>(this));    // モデルローダーを登録
        RegisterLoader(Tsukino::Core::CreateRef<CubemapLoader>());      // キューブマップローダーを登録
        RegisterLoader(Tsukino::Core::CreateRef<EffectLoader>());       // エフェクトローダーを登録

        //--------------------------------------------------------------
        // インポーター登録
        //--------------------------------------------------------------
        RegisterImporter(AssetType::Shader, Tsukino::Core::CreateRef<ShaderImporter>());      // シェーダーインポーターの登録
        RegisterImporter(AssetType::Texture, Tsukino::Core::CreateRef<TextureImporter>());    // テクスチャインポーターを登録
        RegisterImporter(AssetType::Font, Tsukino::Core::CreateRef<FontImporter>());          // フォントインポーターの登録
        RegisterImporter(AssetType::DynamicFont, Tsukino::Core::CreateRef<DynamicFontImporter>());    // 動的フォントインポーターの登録
        RegisterImporter(AssetType::Audio, Tsukino::Core::CreateRef<AudioImporter>());        // オーディオインポーターの登録
        RegisterImporter(AssetType::Model, Tsukino::Core::CreateRef<ModelImporter>());        // モデルインポーターの登録
        RegisterImporter(AssetType::Cubemap, Tsukino::Core::CreateRef<CubemapImporter>());    // キューブマップインポーターを登録
        RegisterImporter(AssetType::Effect, Tsukino::Core::CreateRef<EffectImporter>());      // エフェクトインポーターを登録
    }

    //--------------------------------------------------------------
    //! @brief アセットをロードする関数
    //--------------------------------------------------------------
    AssetHandle AssetManager::Load(const Tsukino::Core::Path& path) {
        //--------------------------------------------------------------
        // 既に同じパスをロード済みならそのハンドルを返す。
        //
        // これが無いと、同じモデルを使うエンティティを生成するたびに
        // キャッシュファイルの読み直し（cerealの全展開）と新しいハンドルの発行が
        // 走り、ModelSystem 側では別モデル扱いになって GPU バッファまで
        // 重複して作られてしまう（敵を大量に湧かせると致命的になる）。
        //--------------------------------------------------------------
        // ハンドルもこのキーから作るため、正規化はAssetHandleGeneratorへ集約する
        // （キャッシュのキーとハンドルの元が食い違うと、同じアセットに別ハンドルが出る）
        const std::string pathKey = AssetHandleGenerator::NormalizeKey(path.string());

        auto cachedIt = m_pathToHandle.find(pathKey);
        if(cachedIt != m_pathToHandle.end())
            return cachedIt->second;

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
        auto importerIt = m_importers.find(type);    // 対応するインポーターを検索
        if(importerIt != m_importers.end()) {
            bool shouldImport = false;    // インポートが必要かどうかのフラグ

            if(!Tsukino::IO::FileSystem::Exists(cacheBasePath)) {
                shouldImport = true;
            } else {
                Tsukino::Core::Path baseDir    = Tsukino::IO::FileSystem::GetAssetRootPath();
                Tsukino::Core::Path sourceFull = baseDir / sourceBasePath;

                if(!Tsukino::IO::FileSystem::Exists(sourceFull)) {
                    // ソースファイルが存在しない（.gitignoreで除外されているなど）
                    // キャッシュをそのまま使用する
                    shouldImport = false;
                } else {
                    // ソースとキャッシュの更新日時を比較
                    auto sourceTime = Tsukino::IO::FileSystem::GetLastWriteTime(sourceFull);
                    auto cacheTime  = Tsukino::IO::FileSystem::GetLastWriteTime(cacheBasePath);

                    if(sourceTime > cacheTime) {
                        // ソースが更新されているので再インポートが必要
                        Tsukino::Core::Log::Info("Asset updated. Re-importing: " + path.string());
                        shouldImport = true;
                    }
                }
            }

            if(shouldImport) {
                Tsukino::Core::Path cacheDir = Tsukino::IO::FileSystem::GetAssetRootPath() / "Cache";

                // 失敗しても即座には抜けない。古いキャッシュが残っていれば
                // それで読み込みを続行できるため。キャッシュも無ければ
                // 下の Loader が失敗して Invalid が返る
                if(!importerIt->second->Import(sourceBasePath, cacheDir)) {
                    Tsukino::Core::Log::Error("AssetManager: Import failed: " + path.string());
                }
            }
        }

        //------------------------------------------------
        // Loader
        //------------------------------------------------
        for(auto& loader : m_loaders) {
            if(loader->CanLoad(cacheBasePath.extension())) {
                // ローダーには fragment 付きで渡す（Audio サブリソース解決用）
                Tsukino::Core::Ref<IAsset> asset = loader->Load(cachePathWithFragment);
                if(!asset)
                    continue;

                AssetHandle handle = AssetHandleGenerator::GenerateFromKey(pathKey);
                asset->SetHandle(handle);
                m_assets.insert({handle.Value(), asset});
                m_pathToHandle.insert({pathKey, handle});
                return handle;
            }
        }

        return AssetHandle::Invalid();
    }

    //--------------------------------------------------------------
    //! @brief 任意のアセットハンドルからアセットを取得する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> AssetManager::Get(AssetHandle handle) {
        auto it = m_assets.find(handle.Value());
        return it != m_assets.end() ? it->second : nullptr;
    }

    //--------------------------------------------------------------
    //! @brief アセットハンドルが存在するか確認する関数
    //--------------------------------------------------------------
    bool AssetManager::Exists(AssetHandle handle) {
        return m_assets.contains(handle.Value());
    }

    //--------------------------------------------------------------
    //! @brief インポーターを登録する関数
    //--------------------------------------------------------------
    void AssetManager::RegisterImporter(AssetType type, Tsukino::Core::Ref<IAssetImporter> importer) {
        if(!importer) {
            Tsukino::Core::Log::Error(std::format("AssetManager::RegisterImporter - Importer is null for AssetType {}", (int)type));
            return;
        }

        if(m_importers.contains(type)) {
            Tsukino::Core::Log::Warn(std::format("AssetManager::RegisterImporter - Importer for AssetType {} is already registered. Overwriting.", (int)type));
        }

        m_importers[type] = importer;
        Tsukino::Core::Log::Info(std::format("Registered importer for AssetType {}", (int)type));
    }

    //--------------------------------------------------------------
    //! @brief アセットを登録する関数
    //--------------------------------------------------------------
    void AssetManager::RegisterAsset(AssetHandle handle, Tsukino::Core::Ref<IAsset> asset) {
        m_assets.insert({handle.Value(), asset});
    }

    //--------------------------------------------------------------
    //! @brief ローダーを登録する関数
    //--------------------------------------------------------------
    void AssetManager::RegisterLoader(Tsukino::Core::Ref<IAssetLoader> loader) {
        m_loaders.push_back(loader);
    }

    //--------------------------------------------------------------
    //! @brief  拡張子からアセットの種類を取得する関数
    //--------------------------------------------------------------
    AssetType AssetManager::GetAssetTypeFromExtension(const std::string& ext) {
        static const std::unordered_map<std::string, AssetType> extensionToAssetType = {
            {".png",     AssetType::Texture},
            {".jpg",     AssetType::Texture},
            {".jpeg",    AssetType::Texture},
            {".bmp",     AssetType::Texture},
            {".tga",     AssetType::Texture},
            {".dds",     AssetType::Texture},

            {".shader",  AssetType::Shader },
            {".hlsl",    AssetType::Shader },

            {".obj",     AssetType::Model  },
            {".fbx",     AssetType::Model  },
            {".gltf",    AssetType::Model  },
            {".glb",     AssetType::Model  },
            {".tsm",     AssetType::Model  },

            {".wav",     AssetType::Audio  },

            {".font",    AssetType::Font   },
            {".dfont",   AssetType::DynamicFont},

            {".cubemap", AssetType::Cubemap},
            {".tcc",     AssetType::Cubemap},

            {".efk",     AssetType::Effect },
            {".efkefc",  AssetType::Effect },
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
            {".hlsl",    ".cso"       },
            {".shader",  ".cso"       },

            {".png",     ".dds"       },
            {".jpg",     ".dds"       },
            {".jpeg",    ".dds"       },
            {".tga",     ".dds"       },
            {".bmp",     ".dds"       },
            {".dds",     ".dds"       },

            {".font",    ".spritefont"},

            {".obj",     ".tsm"       },
            {".fbx",     ".tsm"       },
            {".gltf",    ".tsm"       },
            {".glb",     ".tsm"       },
            {".tsm",     ".tsm"       },

            // Audio
            {".wav",     ".xwb"       },

            // Cubemap
            {".cubemap", ".tcc"       },
            {".tcc",     ".tcc"       },

            {".efk",     ".efk"       },
            {".efkefc",  ".efkefc"    },
        };

        auto [sourceBase, sourceFragment] = Tsukino::Core::Path::SplitPathAndFragment(sourcePath.string());
        Tsukino::Core::Path sourceBasePath(sourceBase);

        // 絶対パス(エンジン自身が所有するTools/やTsukino.BuiltIn/Assetsなど、
        // GetEngineAssetRootPath()経由で解決されたもの)の場合、そのまま
        // GetAssetRootPath() / "Cache" / sourceBasePath とすると、std::filesystem::path の
        // 標準セマンティクスにより絶対パスであるsourceBasePathへ丸ごと置き換わってしまい、
        // エンジンのソースツリー内にキャッシュを書き込んでしまう。
        // それを防ぐため、絶対パスの場合はエンジンルートからの相対パスに変換してから
        // Cache/ 以下に組み込む(相対パスのまま渡される通常のゲームアセットは対象外で挙動は変わらない)。
        Tsukino::Core::Path cacheRelativeSource = Tsukino::IO::FileSystem::ToEngineRelativePath(sourceBasePath);

        Tsukino::Core::Path cacheBasePath = Tsukino::IO::FileSystem::GetAssetRootPath() / "Cache" / cacheRelativeSource;

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

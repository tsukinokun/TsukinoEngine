//--------------------------------------------------------------
//! @file       PrefabFactory.hpp
//! @brief      JSONから任意のコンポーネントをロード・組み立てを行う汎用ファクトリクラス
//! @author     山﨑愛
//--------------------------------------------------------------
#pragma once

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/ECS/EntityRef/EntityRef.hpp>
#include <Tsukino/Core/ECS/EntityRef/EntityRefResolverArchive.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/AssetRef.hpp>
#include <Tsukino/Engine/Asset/AssetRefResolverArchive.hpp>

#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/map.hpp>
#include <entt/entt.hpp>

#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <filesystem>
#include <vector>

// 名前空間 : Tsukino::Engine::ECS::Prefab
namespace Tsukino::Engine::ECS::Prefab {

    //--------------------------------------------------------------
    //! @class  PrefabFactory
    //! @brief  データ駆動のためのPrefabロード＆エンティティ生成システム
    //--------------------------------------------------------------
    class PrefabFactory {
    public:
        //--------------------------------------------------------------
        // 動的ロード関数の型定義 (ラッパーレジストリ、ターゲットエンティティ、コンポーネント単体のJSONパス)
        //--------------------------------------------------------------
        using ComponentLoader = std::function<void(Tsukino::ECS::Registry&, entt::entity, const std::string&)>;

        //--------------------------------------------------------------
        // 動的セーブ関数の型定義（コンポーネントが付いていればJSONへ書き出し、書き出し先パスを返す。付いていなければfalse）
        //--------------------------------------------------------------
        using ComponentSaver = std::function<bool(Tsukino::ECS::Registry&, entt::entity, const std::string&, std::string&)>;

        //--------------------------------------------------------------
        // EntityRef解決関数の型定義（アタッチ済みコンポーネントをEntityRefResolverArchiveで再訪問する）
        //--------------------------------------------------------------
        using ComponentResolver = std::function<void(Tsukino::ECS::Registry&, entt::entity, Tsukino::ECS::EntityRefResolverArchive&)>;

        //--------------------------------------------------------------
        // AssetRef解決関数の型定義（アタッチ済みコンポーネントをAssetRefResolverArchiveで再訪問する）
        //--------------------------------------------------------------
        using ComponentAssetResolver = std::function<void(Tsukino::ECS::Registry&, entt::entity, Tsukino::Asset::AssetRefResolverArchive&)>;

        //--------------------------------------------------------------
        // Instantiate/InstantiateGroupで生成するエンティティ1体の記述（名前 + Prefab目次JSONパス）
        //--------------------------------------------------------------
        struct GroupEntry {
            std::string name;          // バッチ内でEntityRefから参照するための名前
            std::string prefabPath;    // Prefab（目次）JSONパス
        };

        //--------------------------------------------------------------
        // InstantiateGroupの戻り値：バッチ内の名前 -> 生成されたエンティティ の対応表
        //--------------------------------------------------------------
        using PrefabInstance = std::unordered_map<std::string, entt::entity>;

        //--------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //--------------------------------------------------------------
        PrefabFactory() = default;

        //--------------------------------------------------------------
        //! @brief デストラクタ
        //--------------------------------------------------------------
        ~PrefabFactory() = default;

        //--------------------------------------------------------------
        // コピーおよびムーブの制御（意図しない不正コピーを防ぐ）
        //--------------------------------------------------------------
        PrefabFactory(const PrefabFactory&)            = delete;
        PrefabFactory& operator=(const PrefabFactory&) = delete;
        PrefabFactory(PrefabFactory&&)                 = default;
        PrefabFactory& operator=(PrefabFactory&&)      = default;

        //--------------------------------------------------------------
        //! @brief     コンポーネントの型名とロード処理を工場に登録する
        //! @tparam    ComponentType 登録したいコンポーネントの型
        //! @param     typeName      [in] JSON内でキーとなる型名（例: "Transform"）
        //--------------------------------------------------------------
        template <typename ComponentType>
        void RegisterComponent(const std::string& typeName) {
            m_loaders[typeName] = [typeName](Tsukino::ECS::Registry& registry, entt::entity entity, const std::string& compJsonPath) {
                //--------------------------------------------------------------
                // まずはアタッチ（これで初期値が入る）
                //--------------------------------------------------------------
                if(!registry.HasComponent<ComponentType>(entity)) {
                    registry.AddComponent<ComponentType>(entity);
                }

                //--------------------------------------------------------------
                // "null" か空ならロードをスキップ
                //--------------------------------------------------------------
                if(compJsonPath == "null" || compJsonPath.empty()) {
                    return;
                }

                //--------------------------------------------------------------
                // ロード処理（シリアライズ対応型のみ実行）
                // 注意：archive(...)が呼べるかどうかをrequires式で直接判定すると、
                // cereal側の「対応する記法が無い場合のフォールバック実装」が
                // オーバーロード解決自体には成功してしまい、実際に呼び出した瞬間に
                // static_assertで強制終了する（SFINAEで弾かれない）。
                // そのためcereal::traits::is_input_serializableで安全に判定する。
                //--------------------------------------------------------------
                auto& component = registry.GetComponent<ComponentType>(entity);
                if constexpr(cereal::traits::is_input_serializable<ComponentType, cereal::JSONInputArchive>::value) {
                    std::ifstream is(compJsonPath);
                    if(is.is_open()) {
                        cereal::JSONInputArchive archive(is);
                        archive(cereal::make_nvp(typeName, component));
                    } else {
                        Tsukino::Core::Log::Warn("Component JSON not found: " + compJsonPath);
                    }
                }
            };

            //--------------------------------------------------------------
            // セーブ対応型（save()が定義されている型）のみ、ラウンドトリップ保存用の関数も登録する
            //--------------------------------------------------------------
            if constexpr(cereal::traits::is_output_serializable<ComponentType, cereal::JSONOutputArchive>::value) {
                m_savers[typeName] = [typeName](Tsukino::ECS::Registry& registry, entt::entity entity, const std::string& outDir, std::string& outPath) -> bool {
                    if(!registry.HasComponent<ComponentType>(entity)) {
                        return false;
                    }

                    const std::string path = outDir + "/" + typeName + ".json";
                    std::ofstream     os(path);
                    if(!os.is_open()) {
                        Tsukino::Core::Log::Error("Failed to open file for saving: " + path);
                        return false;
                    }

                    const auto& component = registry.GetComponent<ComponentType>(entity);
                    {
                        cereal::JSONOutputArchive archive(os);
                        archive(cereal::make_nvp(typeName, component));
                    }

                    outPath = path;
                    return true;
                };
            }

            //--------------------------------------------------------------
            // EntityRefフィールドを持ちうる型（load()が定義されている型）のみ、
            // InstantiateGroup後の参照解決パスで再訪問できるよう登録する
            //--------------------------------------------------------------
            if constexpr(requires(Tsukino::ECS::EntityRefResolverArchive& archive, ComponentType& component) { load(archive, component); }) {
                m_resolvers[typeName] = [](Tsukino::ECS::Registry& registry, entt::entity entity, Tsukino::ECS::EntityRefResolverArchive& archive) {
                    if(registry.HasComponent<ComponentType>(entity)) {
                        load(archive, registry.GetComponent<ComponentType>(entity));
                    }
                };
            }

            //--------------------------------------------------------------
            // AssetRefフィールドを持ちうる型（load()が定義されている型）のみ、
            // Instantiate直後にその場でパス解決できるよう登録する
            //--------------------------------------------------------------
            if constexpr(requires(Tsukino::Asset::AssetRefResolverArchive& archive, ComponentType& component) { load(archive, component); }) {
                m_assetRefResolvers[typeName] = [](Tsukino::ECS::Registry& registry, entt::entity entity, Tsukino::Asset::AssetRefResolverArchive& archive) {
                    if(registry.HasComponent<ComponentType>(entity)) {
                        load(archive, registry.GetComponent<ComponentType>(entity));
                    }
                };
            }
        }

        //--------------------------------------------------------------
        //! @brief     AssetRefフィールドの解決に使うAssetManagerを設定する
        //! @param     assetManager [in] パスからアセットハンドルを解決するために使うAssetManager
        //--------------------------------------------------------------
        void SetAssetManager(Tsukino::Asset::AssetManager* assetManager) {
            m_assetManager = assetManager;
        }

        //--------------------------------------------------------------
        //! @brief     生成済みエンティティの指定コンポーネントを、任意の値で上書きする
        //! @tparam    T      上書きしたいコンポーネントの型
        //! @param     registry     [in] 対象のレジストリ
        //! @param     entity       [in] 対象のエンティティ
        //! @param     overrideValue [in] 上書きする値（未アタッチなら新規アタッチしてから上書きする）
        //--------------------------------------------------------------
        template <typename T>
        void ApplyOverride(Tsukino::ECS::Registry& registry, entt::entity entity, const T& overrideValue) {
            if(!registry.HasComponent<T>(entity)) {
                registry.AddComponent<T>(entity);
            }
            registry.GetComponent<T>(entity) = overrideValue;
        }

        //--------------------------------------------------------------
        //! @brief     PrefabのJSON（目次）から動的にコンポーネントを組み立ててエンティティを生成する
        //! @param     prefabJsonPath Prefab（目次）のJSONファイルパス
        //! @param     registry       アタッチ先となるラッパーレジストリ
        //! @return    生成されたエンティティ（失敗時は entt::null）
        //--------------------------------------------------------------
        [[nodiscard]]
        entt::entity Instantiate(const std::string& prefabJsonPath, Tsukino::ECS::Registry& registry) {
            return InstantiateInternal(prefabJsonPath, registry, nullptr);
        }

        //--------------------------------------------------------------
        //! @brief     複数のPrefabを名前付きでまとめて生成し、コンポーネント内のEntityRefフィールドを
        //!            バッチ内の他エンティティへ解決する
        //! @param     entries  [in] {名前, Prefab（目次）JSONパス} のリスト
        //! @param     registry [in] アタッチ先となるラッパーレジストリ
        //! @return    名前 -> 生成されたエンティティ の対応表
        //--------------------------------------------------------------
        [[nodiscard]]
        PrefabInstance InstantiateGroup(const std::vector<GroupEntry>& entries, Tsukino::ECS::Registry& registry) {
            PrefabInstance                                              instance;
            std::unordered_map<entt::entity, std::vector<std::string>> attachedTypesByEntity;

            //--------------------------------------------------------------
            // ① 全エンティティを生成し、名前 -> エンティティ の対応表を作る
            //    （この時点でEntityRefはlocalNameのみ保持し、entityは未解決）
            //--------------------------------------------------------------
            for(const auto& entry : entries) {
                std::vector<std::string> typeNames;
                entt::entity             entity = InstantiateInternal(entry.prefabPath, registry, &typeNames);
                if(entity == entt::null) {
                    Tsukino::Core::Log::Error("InstantiateGroup: failed to instantiate '" + entry.name + "' from " + entry.prefabPath);
                    continue;
                }

                instance[entry.name]         = entity;
                attachedTypesByEntity[entity] = std::move(typeNames);
            }

            //--------------------------------------------------------------
            // ② 生成済みの全コンポーネントをEntityRefResolverArchiveで再訪問し、
            //    localNameを実体（entt::entity）へ解決する
            //--------------------------------------------------------------
            Tsukino::ECS::EntityRefResolverArchive resolverArchive(instance);

            for(const auto& [entity, typeNames] : attachedTypesByEntity) {
                for(const auto& typeName : typeNames) {
                    auto it = m_resolvers.find(typeName);
                    if(it != m_resolvers.end()) {
                        it->second(registry, entity, resolverArchive);
                    }
                }
            }

            return instance;
        }

        //--------------------------------------------------------------
        //! @brief     グループ定義JSON（"Entities": {名前: Prefabパス} のマップ）からバッチ生成を行う
        //! @param     groupJsonPath [in] グループ定義JSONファイルパス
        //! @param     registry      [in] アタッチ先となるラッパーレジストリ
        //! @return    名前 -> 生成されたエンティティ の対応表
        //--------------------------------------------------------------
        [[nodiscard]]
        PrefabInstance InstantiateGroup(const std::string& groupJsonPath, Tsukino::ECS::Registry& registry) {
            if(!std::filesystem::exists(groupJsonPath)) {
                Tsukino::Core::Log::Error("Prefab group file not found: " + groupJsonPath);
                return {};
            }

            std::map<std::string, std::string> entityList;
            {
                std::ifstream is(groupJsonPath);
                if(!is.is_open()) {
                    return {};
                }

                cereal::JSONInputArchive archive(is);
                archive(cereal::make_nvp("Entities", entityList));
            }

            std::vector<GroupEntry> entries;
            entries.reserve(entityList.size());
            for(const auto& [name, prefabPath] : entityList) {
                entries.push_back(GroupEntry{name, prefabPath});
            }

            return InstantiateGroup(entries, registry);
        }

        //--------------------------------------------------------------
        //! @brief     生成済みエンティティが持つ、セーブ対応済みコンポーネントをすべてJSONへ書き出し、
        //!            それらをまとめるPrefab（目次）JSONも生成する（開発者向けブートストラップ用途）
        //! @param     registry [in] 対象のレジストリ
        //! @param     entity   [in] 書き出したいエンティティ
        //! @param     outDir   [in] 出力先ディレクトリ（各コンポーネントJSON + Prefab.jsonがここに生成される）
        //! @return    1つ以上のコンポーネントを書き出せたか
        //--------------------------------------------------------------
        bool CaptureEntity(Tsukino::ECS::Registry& registry, entt::entity entity, const std::string& outDir) {
            std::filesystem::create_directories(outDir);

            std::map<std::string, std::string> componentList;
            for(const auto& [typeName, saver] : m_savers) {
                std::string path;
                if(saver(registry, entity, outDir, path)) {
                    componentList[typeName] = path;
                }
            }

            if(componentList.empty()) {
                Tsukino::Core::Log::Warn("CaptureEntity: no serializable components found on entity");
                return false;
            }

            const std::string prefabPath = outDir + "/Prefab.json";
            std::ofstream     os(prefabPath);
            if(!os.is_open()) {
                Tsukino::Core::Log::Error("Failed to open file for saving: " + prefabPath);
                return false;
            }

            cereal::JSONOutputArchive archive(os);
            archive(cereal::make_nvp("Components", componentList));
            return true;
        }

        //--------------------------------------------------------------
        //! @brief     任意のComponent（T）をJSONファイルからロードする（個別用）
        //! @tparam T  ロードしたい構造体の型（例: CameraComponent）
        //! @param     jsonPath  [in] JSONファイルへの相対/絶対パス
        //! @param     keyName   [in] JSON内のルートキー名（例: "CameraPrefab"）
        //! @return    ロードに成功したか（ファイルが開けたらtrue）
        //--------------------------------------------------------------
        template <typename T>
        [[nodiscard]] bool Load(const std::string& jsonPath, const std::string& keyName, T& outData) {
            std::ifstream is(jsonPath);

            if(!is.is_open()) {
                Tsukino::Core::Log::Warn("Prefab file not found: " + jsonPath + " (Using default parameters)");
                return false;
            }

            {
                cereal::JSONInputArchive archive(is);
                archive(cereal::make_nvp(keyName, outData));
            }

            return true;
        }

        //--------------------------------------------------------------
        //! @brief     任意のComponent（T）をJSONファイルにセーブする（個別用）
        //! @tparam T  セーブしたいComponentの型
        //! @param     jsonPath [in] 保存先のJSONファイルパス
        //! @param     keyName  [in] JSON内のルートキー名
        //! @param     desc     [in] 保存したいComponentデータ
        //! @return    セーブに成功したか
        //--------------------------------------------------------------
        template <typename T>
        bool Save(const std::string& jsonPath, const std::string& keyName, const T& desc) {
            std::ofstream os(jsonPath);

            if(!os.is_open()) {
                Tsukino::Core::Log::Error("Failed to open file for saving: " + jsonPath);
                return false;
            }

            {
                cereal::JSONOutputArchive archive(os);
                archive(cereal::make_nvp(keyName, desc));
            }

            return true;
        }

    private:
        //--------------------------------------------------------------
        //! @brief     Instantiate/InstantiateGroup共通の内部実装
        //! @param     prefabJsonPath  [in]  Prefab（目次）のJSONファイルパス
        //! @param     registry        [in]  アタッチ先となるラッパーレジストリ
        //! @param     outTypeNames    [out] 非null時、アタッチされたコンポーネントの型名一覧を書き出す
        //! @return    生成されたエンティティ（失敗時は entt::null）
        //--------------------------------------------------------------
        entt::entity InstantiateInternal(const std::string& prefabJsonPath, Tsukino::ECS::Registry& registry, std::vector<std::string>* outTypeNames) {
            if(!std::filesystem::exists(prefabJsonPath)) {
                Tsukino::Core::Log::Error("Prefab file not found: " + prefabJsonPath);
                return entt::null;
            }

            //--------------------------------------------------------------
            // メタデータ（コンポーネント名とパスのマップ）をJSONから読み込む
            //--------------------------------------------------------------
            std::map<std::string, std::string> componentList;
            {
                std::ifstream is(prefabJsonPath);
                if(!is.is_open())
                    return entt::null;

                cereal::JSONInputArchive archive(is);
                archive(cereal::make_nvp("Components", componentList));
            }

            //--------------------------------------------------------------
            // 新しい空のエンティティを生み出す
            //--------------------------------------------------------------
            entt::entity newEntity = registry.CreateEntity();

            //--------------------------------------------------------------
            // JSONに書かれていたコンポーネントをループで自動アタッチ＆ロード
            //--------------------------------------------------------------
            for(const auto& [typeName, compJsonPath] : componentList) {
                auto it = m_loaders.find(typeName);
                if(it != m_loaders.end()) {
                    // 登録されていたラムダを実行
                    it->second(registry, newEntity, compJsonPath);
                    if(outTypeNames) {
                        outTypeNames->push_back(typeName);
                    }

                    //--------------------------------------------------------------
                    // AssetRefフィールドを持つ型なら、その場でパスをハンドルへ解決する
                    // （EntityRefと異なりバッチ内の順序に依存しないため即時解決でよい）
                    //--------------------------------------------------------------
                    if(m_assetManager) {
                        auto assetIt = m_assetRefResolvers.find(typeName);
                        if(assetIt != m_assetRefResolvers.end()) {
                            Tsukino::Asset::AssetRefResolverArchive assetArchive(*m_assetManager);
                            assetIt->second(registry, newEntity, assetArchive);
                        }
                    }
                } else {
                    Tsukino::Core::Log::Warn("Unknown component type written in Prefab: " + typeName + " (Did you forget to register?)");
                }
            }

            return newEntity;
        }

        // 各コンポーネントの文字列キーと、アタッチ＆個別ロード用ラムダを保持するマップ
        std::unordered_map<std::string, ComponentLoader> m_loaders;

        // 各コンポーネントの文字列キーと、ラウンドトリップ保存用ラムダを保持するマップ（save()対応型のみ）
        std::unordered_map<std::string, ComponentSaver> m_savers;

        // 各コンポーネントの文字列キーと、EntityRef解決用ラムダを保持するマップ（load()対応型のみ）
        std::unordered_map<std::string, ComponentResolver> m_resolvers;

        // 各コンポーネントの文字列キーと、AssetRef解決用ラムダを保持するマップ（load()対応型のみ）
        std::unordered_map<std::string, ComponentAssetResolver> m_assetRefResolvers;

        // AssetRef解決に使うAssetManager（SetAssetManagerで設定されるまではnullptrで、解決をスキップする）
        Tsukino::Asset::AssetManager* m_assetManager = nullptr;
    };

}    // namespace Tsukino::Engine::ECS::Prefab

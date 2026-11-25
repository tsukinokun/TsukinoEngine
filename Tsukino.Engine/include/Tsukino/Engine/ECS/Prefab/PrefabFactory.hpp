//--------------------------------------------------------------
//! @file       PrefabFactory.hpp
//! @brief      JSONから任意のコンポーネントをロード・組み立てを行う汎用ファクトリクラス
//! @author     山﨑愛
//--------------------------------------------------------------
#pragma once

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/ECS/EntityRef/EntityRef.hpp>
#include <Tsukino/Core/ECS/EntityRef/EntityRefResolverArchive.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/AssetRef.hpp>

#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/external/rapidjson/stringbuffer.h>
#include <cereal/external/rapidjson/writer.h>
#include <cereal/types/map.hpp>
#include <entt/entt.hpp>

#include <exception>
#include <string>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <utility>
#include <vector>

// 名前空間 : Tsukino::Engine::ECS::Prefab
namespace Tsukino::Engine::ECS::Prefab {

    //--------------------------------------------------------------
    //! @class  PrefabFactory
    //! @brief  データ駆動のためのPrefabロード＆エンティティ生成システム
    //! @note   JSON は2つの書き方を読める。
    //!         - 分割形式：Prefab は {"Components":[{"key":"型名","value":"コンポーネントのJSONパス"}, …]}、
    //!           グループは {"Entities":[{"key":"名前","value":"PrefabのJSONパス"}, …]}（CaptureEntity もこの形で書く）
    //!         - 1ファイル形式：Prefab は {"Components":{"型名":{中身}, …}}、
    //!           グループは {"Entities":{"名前":{"Components":{…}}, …}}。値に文字列を書けば分割形式と同じくパスとして読む
    //--------------------------------------------------------------
    class PrefabFactory {
    public:
        //--------------------------------------------------------------
        //! @struct ComponentSource
        //! @brief  コンポーネント1つ分の読み込み元（ファイルか、Prefab に直接書かれた中身）
        //--------------------------------------------------------------
        struct ComponentSource {
            std::string path;          // コンポーネント単体の JSON ファイル（"null" か空なら初期値のまま）
            std::string inlineJson;    // 1ファイル形式で直接書かれた中身（{"型名": {…}} の形）。空でなければこちらを読む
        };

        //--------------------------------------------------------------
        // 動的ロード関数の型定義 (ラッパーレジストリ、ターゲットエンティティ、コンポーネントの読み込み元)
        //--------------------------------------------------------------
        using ComponentLoader = std::function<void(Tsukino::ECS::Registry&, entt::entity, const ComponentSource&)>;

        //--------------------------------------------------------------
        // 動的セーブ関数の型定義（コンポーネントが付いていればJSONへ書き出し、書き出し先パスを返す。付いていなければfalse）
        //--------------------------------------------------------------
        using ComponentSaver = std::function<bool(Tsukino::ECS::Registry&, entt::entity, const std::string&, std::string&)>;

        //--------------------------------------------------------------
        // EntityRef解決関数の型定義（アタッチ済みコンポーネントをEntityRefResolverArchiveで再訪問する）
        //--------------------------------------------------------------
        using ComponentResolver = std::function<void(Tsukino::ECS::Registry&, entt::entity, Tsukino::ECS::EntityRefResolverArchive&)>;

        //--------------------------------------------------------------
        // Instantiate/InstantiateGroupで生成するエンティティ1体の記述（名前 + Prefab目次JSONパス）
        //--------------------------------------------------------------
        struct GroupEntry {
            std::string name;          // バッチ内でEntityRefから参照するための名前
            std::string prefabPath;    // Prefab（目次）JSONパス
            std::string prefabJson;    // 1ファイル形式で直接書かれた Prefab の中身（{"Components": …}）。空でなければ prefabPath の代わりに使う
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
        PrefabFactory(const PrefabFactory&)                = delete;
        PrefabFactory& operator=(const PrefabFactory&)     = delete;
        PrefabFactory(PrefabFactory&&) noexcept            = default;
        PrefabFactory& operator=(PrefabFactory&&) noexcept = default;

        //--------------------------------------------------------------
        //! @brief     コンポーネントの型名とロード処理を工場に登録する
        //! @tparam    ComponentType 登録したいコンポーネントの型
        //! @param     typeName      [in] JSON内でキーとなる型名（例: "Transform"）
        //--------------------------------------------------------------
        template <typename ComponentType>
        void RegisterComponent(const std::string& typeName) {
            m_loaders[typeName] = [typeName](Tsukino::ECS::Registry& registry, entt::entity entity, const ComponentSource& source) {
                //--------------------------------------------------------------
                // まずはアタッチ（これで初期値が入る）
                //--------------------------------------------------------------
                if(!registry.HasComponent<ComponentType>(entity)) {
                    registry.AddComponent<ComponentType>(entity);
                }

                //--------------------------------------------------------------
                // 中身が無い（パスが "null" か空で、直接書かれた中身も無い）ならロードをスキップ
                //--------------------------------------------------------------
                const bool isInline = !source.inlineJson.empty();
                if(!isInline && (source.path == "null" || source.path.empty())) {
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
                    std::istringstream is;
                    if(isInline) {
                        is.str(source.inlineJson);
                    } else if(!ReadJsonText(source.path, is)) {
                        Tsukino::Core::Log::Warn("Component JSON not found: " + source.path);
                        return;
                    }

                    //--------------------------------------------------------------
                    // load() が読む項目が JSON に無いと cereal は例外を投げる。
                    // Prefab の書き間違いでゲームごと落ちないよう、どの型のどこが悪いかをログに出して続ける
                    //--------------------------------------------------------------
                    try {
                        cereal::JSONInputArchive archive(is);
                        archive(cereal::make_nvp(typeName, component));
                    } catch(const std::exception& e) {
                        Tsukino::Core::Log::Error("Failed to load " + typeName + " from " + (isInline ? std::string("the Prefab JSON") : source.path) +
                                                  ": " + e.what() + " (every field read by load() must be written)");
                    }
                }
            };

            //--------------------------------------------------------------
            // セーブ対応型（save()が定義されている型）のみ、ラウンドトリップ保存用の関数も登録する
            //--------------------------------------------------------------
            if constexpr(cereal::traits::is_output_serializable<ComponentType, cereal::JSONOutputArchive>::value) {
                m_savers[typeName] = [typeName](
                                         Tsukino::ECS::Registry& registry, entt::entity entity, const std::string& outDir, std::string& outPath) -> bool {
                    if(!registry.HasComponent<ComponentType>(entity)) {
                        return false;
                    }

                    const std::string  path = outDir + "/" + typeName + ".json";
                    std::ostringstream os;

                    const auto& component = registry.GetComponent<ComponentType>(entity);
                    {
                        cereal::JSONOutputArchive archive(os);
                        archive(cereal::make_nvp(typeName, component));
                    }

                    if(!WriteJsonText(path, os.str())) {
                        return false;
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
        }

        //--------------------------------------------------------------
        //! @brief     AssetRefフィールドの解決に使うAssetManagerを設定する
        //! @param     assetManager [in] パスからアセットハンドルを解決するために使うAssetManager
        //--------------------------------------------------------------
        void SetAssetManager(Tsukino::Asset::AssetManager* assetManager) { m_assetManager = assetManager; }

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
            PrefabInstance                                             instance;
            std::unordered_map<entt::entity, std::vector<std::string>> attachedTypesByEntity;

            //--------------------------------------------------------------
            // ① 全エンティティを生成し、名前 -> エンティティ の対応表を作る
            //    （この時点でEntityRefはlocalNameのみ保持し、entityは未解決）
            //--------------------------------------------------------------
            for(const auto& entry : entries) {
                std::vector<std::string> typeNames;
                entt::entity             entity = entry.prefabJson.empty() ? InstantiateInternal(entry.prefabPath, registry, &typeNames)
                                                                           : InstantiateFromText(entry.prefabJson, "'" + entry.name + "'", registry, &typeNames);
                if(entity == entt::null) {
                    Tsukino::Core::Log::Error("InstantiateGroup: failed to instantiate '" + entry.name + "'" +
                                              (entry.prefabJson.empty() ? " from " + entry.prefabPath : std::string()));
                    continue;
                }

                instance[entry.name]          = entity;
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
        //! @brief     グループ定義JSONからバッチ生成を行う
        //! @param     groupJsonPath [in] グループ定義JSONファイルパス
        //! @param     registry      [in] アタッチ先となるラッパーレジストリ
        //! @return    名前 -> 生成されたエンティティ の対応表
        //! @note      "Entities" は分割形式（[{"key":名前,"value":Prefabパス}, …]）と
        //!            1ファイル形式（{名前: Prefabの中身 か Prefabパス, …}）のどちらでもよい
        //--------------------------------------------------------------
        [[nodiscard]]
        PrefabInstance InstantiateGroup(const std::string& groupJsonPath, Tsukino::ECS::Registry& registry) {
            std::istringstream is;
            if(!ReadJsonText(groupJsonPath, is)) {
                Tsukino::Core::Log::Error("Prefab group file not found: " + groupJsonPath);
                return {};
            }

            JsonDocument document;
            if(!ParseJson(is.str(), groupJsonPath, document)) {
                return {};
            }
            if(!document.HasMember("Entities")) {
                Tsukino::Core::Log::Error("Prefab group has no \"Entities\": " + groupJsonPath);
                return {};
            }

            std::vector<GroupEntry> entries;
            if(!ReadNamedEntries(document["Entities"], groupJsonPath, "Entities", [&entries, &groupJsonPath](const std::string& name, const JsonValue& value) {
                   if(value.IsObject()) {
                       entries.push_back(GroupEntry{name, std::string(), ToJsonText(value)});
                   } else if(value.IsString()) {
                       entries.push_back(GroupEntry{name, std::string(value.GetString(), value.GetStringLength()), std::string()});
                   } else {
                       Tsukino::Core::Log::Error("InstantiateGroup: '" + name + "' in " + groupJsonPath + " has no Prefab (null)");
                   }
               })) {
                return {};
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

            const std::string  prefabPath = outDir + "/Prefab.json";
            std::ostringstream os;
            {
                cereal::JSONOutputArchive archive(os);
                archive(cereal::make_nvp("Components", componentList));
            }
            return WriteJsonText(prefabPath, os.str());
        }

        //--------------------------------------------------------------
        //! @brief     任意のComponent（T）をJSONファイルからロードする（個別用）
        //! @tparam T  ロードしたい構造体の型（例: CameraComponent）
        //! @param     jsonPath  [in] JSONファイルへの相対/絶対パス
        //! @param     keyName   [in] JSON内のルートキー名（例: "CameraPrefab"）
        //! @param     outData   [out] 読み込んだ値の書き込み先
        //! @return    ロードに成功したか（ファイルが開けたらtrue）
        //--------------------------------------------------------------
        template <typename T>
        [[nodiscard]] bool Load(const std::string& jsonPath, const std::string& keyName, T& outData) {
            std::istringstream is;
            if(!ReadJsonText(jsonPath, is)) {
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
            std::ostringstream os;
            {
                cereal::JSONOutputArchive archive(os);
                archive(cereal::make_nvp(keyName, desc));
            }
            return WriteJsonText(jsonPath, os.str());
        }

    private:
        //--------------------------------------------------------------
        //! @brief     JSON ファイルを読み、読み込み用のストリームへ入れる
        //! @param     path [in]  ファイルのパス（UTF-8）
        //! @param     out  [out] 中身を入れる先
        //! @return    ファイルがあれば true
        //--------------------------------------------------------------
        static bool ReadJsonText(const std::string& path, std::istringstream& out) {
            const Tsukino::Core::Path filePath(path);
            if(!Tsukino::IO::FileSystem::Exists(filePath)) {
                return false;
            }
            out.str(Tsukino::IO::FileSystem::ReadText(filePath));
            return true;
        }

        //--------------------------------------------------------------
        //! @brief     JSON の文字列をファイルへ書く（足りないフォルダは作る）
        //! @param     path [in] ファイルのパス（UTF-8）
        //! @param     text [in] 書く文字列
        //! @return    書けたら true（書けなければエラーをログへ出す）
        //--------------------------------------------------------------
        static bool WriteJsonText(const std::string& path, const std::string& text) {
            if(!Tsukino::IO::FileSystem::WriteText(Tsukino::Core::Path(path), text)) {
                Tsukino::Core::Log::Error("Failed to open file for saving: " + path);
                return false;
            }
            return true;
        }

        // cereal が同梱している rapidjson の型（cereal の JSON アーカイブと同じものを使う）
        using JsonDocument = CEREAL_RAPIDJSON_NAMESPACE::Document;
        using JsonValue    = CEREAL_RAPIDJSON_NAMESPACE::Value;

        //--------------------------------------------------------------
        //! @brief     JSON の文字列をパースする
        //! @param     text     [in]  JSON の文字列
        //! @param     where    [in]  エラーに出す場所の名前（ファイルパスなど）
        //! @param     document [out] パースした結果
        //! @return    ルートがオブジェクトとして読めたら true（読めなければエラーをログへ出す）
        //--------------------------------------------------------------
        static bool ParseJson(const std::string& text, const std::string& where, JsonDocument& document) {
            document.Parse(text.c_str(), text.size());
            if(document.HasParseError()) {
                Tsukino::Core::Log::Error("Invalid JSON in " + where + " (near byte " + std::to_string(document.GetErrorOffset()) + ")");
                return false;
            }
            if(!document.IsObject()) {
                Tsukino::Core::Log::Error("The root of " + where + " must be a JSON object");
                return false;
            }
            return true;
        }

        //--------------------------------------------------------------
        //! @brief     JSON の値を1行の文字列に戻す
        //! @param     value [in] 値
        //! @return    JSON の文字列
        //--------------------------------------------------------------
        static std::string ToJsonText(const JsonValue& value) {
            CEREAL_RAPIDJSON_NAMESPACE::StringBuffer                                 buffer;
            CEREAL_RAPIDJSON_NAMESPACE::Writer<CEREAL_RAPIDJSON_NAMESPACE::StringBuffer> writer(buffer);
            value.Accept(writer);
            return std::string(buffer.GetString(), buffer.GetSize());
        }

        //--------------------------------------------------------------
        //! @brief     「名前 → 値」の並びを、分割形式・1ファイル形式のどちらからでも読む
        //! @param     list     [in] "Components" か "Entities" の値
        //! @param     where    [in] エラーに出す場所の名前
        //! @param     listName [in] エラーに出す項目名
        //! @param     visit    [in] 1件ごとに (名前, 値) で呼ぶ関数。値は文字列（パス）・オブジェクト（中身）・null のいずれか
        //! @return    形が正しければ true（正しくなければエラーをログへ出す）
        //! @note      分割形式は cereal が std::map を書き出す形（[{"key":…,"value":…}, …]）
        //--------------------------------------------------------------
        template <typename Visitor>
        static bool ReadNamedEntries(const JsonValue& list, const std::string& where, const char* listName, Visitor&& visit) {
            const auto isUsable = [](const JsonValue& value) { return value.IsString() || value.IsObject() || value.IsNull(); };

            if(list.IsObject()) {
                for(auto it = list.MemberBegin(); it != list.MemberEnd(); ++it) {
                    if(!isUsable(it->value)) {
                        Tsukino::Core::Log::Error(std::string("\"") + listName + "\" entry '" + it->name.GetString() + "' in " + where +
                                                  " must be an object or a file path");
                        return false;
                    }
                    visit(std::string(it->name.GetString(), it->name.GetStringLength()), it->value);
                }
                return true;
            }

            if(list.IsArray()) {
                for(const JsonValue& element : list.GetArray()) {
                    if(!element.IsObject() || !element.HasMember("key") || !element["key"].IsString() || !element.HasMember("value") ||
                       !isUsable(element["value"])) {
                        Tsukino::Core::Log::Error(std::string("\"") + listName + "\" in " + where + " must hold {\"key\": name, \"value\": …} entries");
                        return false;
                    }
                    visit(std::string(element["key"].GetString(), element["key"].GetStringLength()), element["value"]);
                }
                return true;
            }

            Tsukino::Core::Log::Error(std::string("\"") + listName + "\" in " + where + " must be an object or an array");
            return false;
        }

        //--------------------------------------------------------------
        //! @brief     Instantiate/InstantiateGroup共通の内部実装（Prefab のファイルから作る）
        //! @param     prefabJsonPath  [in]  Prefab（目次）のJSONファイルパス
        //! @param     registry        [in]  アタッチ先となるラッパーレジストリ
        //! @param     outTypeNames    [out] 非null時、アタッチされたコンポーネントの型名一覧を書き出す
        //! @return    生成されたエンティティ（失敗時は entt::null）
        //--------------------------------------------------------------
        entt::entity InstantiateInternal(const std::string& prefabJsonPath, Tsukino::ECS::Registry& registry, std::vector<std::string>* outTypeNames) {
            std::istringstream is;
            if(!ReadJsonText(prefabJsonPath, is)) {
                Tsukino::Core::Log::Error("Prefab file not found: " + prefabJsonPath);
                return entt::null;
            }
            return InstantiateFromText(is.str(), prefabJsonPath, registry, outTypeNames);
        }

        //--------------------------------------------------------------
        //! @brief     Prefab の中身（JSON の文字列）からエンティティを作る
        //! @param     prefabJson   [in]  Prefab の JSON（{"Components": …}）
        //! @param     where        [in]  エラーに出す場所の名前
        //! @param     registry     [in]  アタッチ先となるラッパーレジストリ
        //! @param     outTypeNames [out] 非null時、アタッチされたコンポーネントの型名一覧を書き出す
        //! @return    生成されたエンティティ（失敗時は entt::null）
        //--------------------------------------------------------------
        entt::entity InstantiateFromText(const std::string&        prefabJson,
                                         const std::string&        where,
                                         Tsukino::ECS::Registry&   registry,
                                         std::vector<std::string>* outTypeNames) {
            //--------------------------------------------------------------
            // コンポーネントの一覧（型名 → 読み込み元）をJSONから読み込む
            //--------------------------------------------------------------
            JsonDocument document;
            if(!ParseJson(prefabJson, where, document)) {
                return entt::null;
            }
            if(!document.HasMember("Components")) {
                Tsukino::Core::Log::Error("Prefab has no \"Components\": " + where);
                return entt::null;
            }

            std::vector<std::pair<std::string, ComponentSource>> componentList;
            const bool                                           listOk =
                ReadNamedEntries(document["Components"], where, "Components", [&componentList](const std::string& typeName, const JsonValue& value) {
                    ComponentSource source;
                    if(value.IsObject()) {
                        // 空のオブジェクトは「初期値のまま付ける」（"null" と同じ）
                        if(value.MemberCount() > 0) {
                            source.inlineJson = "{\"" + typeName + "\":" + ToJsonText(value) + "}";
                        }
                    } else if(value.IsString()) {
                        source.path = std::string(value.GetString(), value.GetStringLength());
                    }
                    componentList.emplace_back(typeName, std::move(source));
                });
            if(!listOk) {
                return entt::null;
            }

            //--------------------------------------------------------------
            // 新しい空のエンティティを生み出す
            //--------------------------------------------------------------
            entt::entity newEntity = registry.CreateEntity();

            //--------------------------------------------------------------
            // AssetRefフィールドは、JSONから読むその場でパスをハンドルへ解決させる
            // （EntityRefと異なりバッチ内の順序に依存しないため即時解決でよい）。
            // load()を擬似アーカイブで再訪問して解決すると、hlslppのベクトル等の
            // 「ローカルへ読んで代入」するload()が0で上書きしてしまうため、再訪問はしない
            //--------------------------------------------------------------
            const Tsukino::Asset::ScopedAssetRefResolver assetRefResolver(m_assetManager);

            //--------------------------------------------------------------
            // JSONに書かれていたコンポーネントをループで自動アタッチ＆ロード
            //--------------------------------------------------------------
            for(const auto& [typeName, source] : componentList) {
                auto it = m_loaders.find(typeName);
                if(it != m_loaders.end()) {
                    // 登録されていたラムダを実行
                    it->second(registry, newEntity, source);
                    if(outTypeNames) {
                        outTypeNames->push_back(typeName);
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

        // AssetRef解決に使うAssetManager（SetAssetManagerで設定されるまではnullptrで、解決をスキップする）
        Tsukino::Asset::AssetManager* m_assetManager = nullptr;
    };

}    // namespace Tsukino::Engine::ECS::Prefab

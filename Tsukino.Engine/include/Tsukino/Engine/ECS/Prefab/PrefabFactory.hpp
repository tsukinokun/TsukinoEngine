//--------------------------------------------------------------
//! @file       PrefabFactory.hpp
//! @brief      JSONから任意のコンポーネントをロード・組み立てを行う汎用ファクトリクラス
//! @author     山﨑愛
//--------------------------------------------------------------
#pragma once

#include <Tsukino/Core/Log.hpp>

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

// 名前空間 : Tsukino::Engine::ECS::Prefab
namespace Tsukino::Engine::ECS::Prefab {

    //--------------------------------------------------------------
    //! @class  PrefabFactory
    //! @brief  データ駆動のためのPrefabロード＆エンティティ生成システム（インスタンス駆動版）
    //--------------------------------------------------------------
    class PrefabFactory {
    public:
        //--------------------------------------------------------------
        // 動的ロード関数の型定義 (レジストリ、ターゲットエンティティ、コンポーネント単体のJSONパス)
        //--------------------------------------------------------------
        using ComponentLoader = std::function<void(entt::registry&, entt::entity, const std::string&)>;

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
        //! @param     typeName      JSON内でキーとなる型名（例: "Transform"）
        //--------------------------------------------------------------
        template <typename ComponentType>
        void RegisterComponent(const std::string& typeName) {
            // 💡 [typeName] を追加して、文字列をラムダの内部にコピーして保持させる
            m_loaders[typeName] = [typeName](entt::registry& registry, entt::entity entity, const std::string& compJsonPath) {
                // コンポーネントをエンティティにアタッチ（既に存在する場合は取得）
                auto& component = registry.get_or_emplace<ComponentType>(entity);

                std::ifstream is(compJsonPath);
                if(is.is_open()) {
                    cereal::JSONInputArchive archive(is);
                    archive(cereal::make_nvp(typeName, component));
                } else {
                    Tsukino::Core::Log::Warn("Component JSON not found: " + compJsonPath);
                }
            };
        }

        //--------------------------------------------------------------
        //! @brief     PrefabのJSON（目次）から動的にコンポーネントを組み立ててエンティティを生成する
        //! @param     prefabJsonPath Prefab（目次）のJSONファイルパス
        //! @param     registry       アタッチ先となるEnTTのレジストリ
        //! @return    生成されたエンティティ（失敗時は entt::null）
        //--------------------------------------------------------------
        [[nodiscard]]
        entt::entity Instantiate(const std::string& prefabJsonPath, entt::registry& registry) {
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
            entt::entity newEntity = registry.create();

            //--------------------------------------------------------------
            // JSONに書かれていたコンポーネントをループで自動アタッチ＆ロード
            //--------------------------------------------------------------
            for(const auto& [typeName, compJsonPath] : componentList) {
                auto it = m_loaders.find(typeName);
                if(it != m_loaders.end()) {
                    // 登録されていたラムダを実行
                    it->second(registry, newEntity, compJsonPath);
                } else {
                    Tsukino::Core::Log::Warn("Unknown component type written in Prefab: " + typeName + " (Did you forget to register?)");
                }
            }

            return newEntity;
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
        // 各コンポーネントの文字列キーと、アタッチ＆個別ロード用ラムダを保持するマップ
        std::unordered_map<std::string, ComponentLoader> m_loaders;
    };

}    // namespace Tsukino::Engine::ECS::Prefab

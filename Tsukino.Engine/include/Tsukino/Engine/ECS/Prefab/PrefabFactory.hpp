//--------------------------------------------------------------
//! @file   PrefabFactory.hpp
//! @brief  JSONから任意のDescをロードする汎用ファクトリクラス
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Log.hpp>

#include <cereal/cereal.hpp>

#include <string>
#include <fstream>
#include <iostream>
// 名前空間 : Tsukino::Engine::ECS::Prefab
namespace Tsukino::Engine::ECS::Prefab {
    //--------------------------------------------------------------
    //! @class  PrefabFactory
    //! @brief  データ駆動のためのPrefabロードシステム（静的関数のみ）
    //--------------------------------------------------------------
    class PrefabFactory {
    public:
        //--------------------------------------------------------------
        // 静的ユーティリティクラスとして扱うため、コンストラクタ等は禁止
        //--------------------------------------------------------------

        //--------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //--------------------------------------------------------------
        PrefabFactory()  = delete;

        //--------------------------------------------------------------
        //! @brief デストラクタ
        //--------------------------------------------------------------
        ~PrefabFactory() = delete;

        //--------------------------------------------------------------
        //! @brief     任意のDesc（T）をJSONファイルからロードする
        //! @tparam T  ロードしたいDesc構造体の型（例: CameraDesc）
        //! @param     jsonPath  [in] JSONファイルへの相対/絶対パス
        //! @param     keyName   [in]JSON内のルートキー名（例: "CameraPrefab"）
        //! @return    ロードされたDescデータ（失敗時はデフォルト値が返る）
        //--------------------------------------------------------------
        template <typename T>
        static [[nodiscard]] T LoadDesc(const std::string& jsonPath, const std::string& keyName) {
            //--------------------------------------------------------------
            // descを宣言して、jsonファイルを開く
            //--------------------------------------------------------------
            T             desc;
            std::ifstream is(jsonPath);

            //--------------------------------------------------------------
            // ファイルが開けなかった場合は、構造体のデフォルト値をそのまま返す
            //--------------------------------------------------------------
            if(!is.is_open()) {
                Tsukino::Core::Log::Warn("Prefab file not found: " + jsonPath + " (Using default parameters)");
                return desc;
            }

            //--------------------------------------------------------------
            // JSONのパースとDescへの展開
            //--------------------------------------------------------------
            //{
            //    cereal::JSONInputArchive archive(is);
            //    archive(cereal::make_nvp(keyName, desc));
            //}

            // ロードに成功していれば、JSONから展開された値が入ったdescを返す
            return desc;
        }

        //--------------------------------------------------------------
        //! @brief     任意のDesc（T）をJSONファイルにセーブする
        //! @tparam T  セーブしたいDesc構造体の型
        //! @param     jsonPath [in] 保存先のJSONファイルパス
        //! @param     keyName  [in] JSON内のルートキー名
        //! @param     desc     [in] 保存したいDescデータ
        //! @return    セーブに成功したか
        //--------------------------------------------------------------
        template <typename T>
        static bool SaveDesc(const std::string& jsonPath, const std::string& keyName, const T& desc) {
            // JSONファイルを開く（上書きモード）
            std::ofstream os(jsonPath);

            //--------------------------------------------------------------
            // ファイルが開けなかった場合はエラーメッセージを出して失敗を返す
            //--------------------------------------------------------------
            if(!os.is_open()) {
                Tsukino::Core::Log::Error("Failed to open file for saving: " + jsonPath);
                return false;
            }

            //--------------------------------------------------------------
            //　JSONへのシリアライズとファイルへの書き込み
            //--------------------------------------------------------------
            //{
            //    cereal::JSONOutputArchive archive(os);
            //    archive(cereal::make_nvp(keyName, desc));
            //}

            return true;
        }
    };

}    // namespace Tsukino::Engine::ECS::Prefab

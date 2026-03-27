//-------------------------------------------------------------
//! @file    GameSceneManager.hpp
//! @brief   ゲームシーン管理クラスの宣言
//! @author  山﨑愛
//-------------------------------------------------------------
#pragma once
#include <memory>
// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {
    // 前方宣言
    class GameSceneBase;
    class EngineAPI;
    //-------------------------------------------------------------
    //! @class   GameSceneManager
    //! @brief   ゲームシーンの進行と切り替えを管理するクラス
    //-------------------------------------------------------------
    class GameSceneManager {
    public:
        //-------------------------------------------------------------
        // デフォルトコンストラクタ
        //-------------------------------------------------------------
        GameSceneManager() = default;

        //-------------------------------------------------------------
        // デストラクタ
        //-------------------------------------------------------------
        ~GameSceneManager();

        //-------------------------------------------------------------
        // コピー・ムーブの禁止
        //-------------------------------------------------------------
        GameSceneManager(const GameSceneManager&)            = delete;
        GameSceneManager& operator=(const GameSceneManager&) = delete;
        GameSceneManager(GameSceneManager&&)                 = delete;
        GameSceneManager& operator=(GameSceneManager&&)      = delete;

        //-------------------------------------------------------------
        // 次のシーンへの遷移を予約する
        //! @param  newScene [in] 次に遷移するシーンのインスタンス
        //-------------------------------------------------------------
        void ChangeScene(std::unique_ptr<GameSceneBase> newScene);

        //-------------------------------------------------------------
        // シーンの更新（毎フレーム呼び出す）
        //! @param  api       [in] エンジンから提供されるAPIへの参照
        //! @param  deltaTime [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        void Update(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime);

        //-------------------------------------------------------------
        // 現在アクティブなシーンを取得する
        //! @return 現在のシーンへのポインタ（存在しない場合はnullptr）
        //-------------------------------------------------------------
        GameSceneBase* GetCurrentScene() const;

    private:
        std::unique_ptr<GameSceneBase> m_currentScene;    // 現在のシーン
        std::unique_ptr<GameSceneBase> m_nextScene;       // 次に遷移予約されたシーン
    };
}    // namespace Tsukino::EngineIntegration

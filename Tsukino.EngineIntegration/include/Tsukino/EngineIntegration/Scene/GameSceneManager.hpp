//-------------------------------------------------------------
//! @file    GameSceneManager.hpp
//! @brief   ゲームシーン管理クラスの宣言
//! @author  山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Event/ScopedConnection.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {
    // 前方宣言
    class GameSceneBase;
    class EngineAPI;
    struct EngineContext;

    //-------------------------------------------------------------
    //! @class   GameSceneManager
    //! @brief   ゲームシーンの進行と切り替えを管理するクラス
    //-------------------------------------------------------------
    class GameSceneManager {
    public:
        //! 名前で切り替えるシーンを作る関数
        using SceneFactory = std::function<std::unique_ptr<GameSceneBase>()>;
        //-------------------------------------------------------------
        // コンストラクタ
        //-------------------------------------------------------------
        GameSceneManager();

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
        // 初期化
        //! @param context [in] エンジン全体で共有されるコンテキスト
        //-------------------------------------------------------------
        void Initialize(EngineContext* context);

        //-------------------------------------------------------------
        // 次のシーンへの遷移を予約する
        //! @param  newScene [in] 次に遷移するシーンのインスタンス
        //-------------------------------------------------------------
        void ChangeScene(std::unique_ptr<GameSceneBase> newScene);

        //-------------------------------------------------------------
        // 名前で切り替えられるシーンを登録する
        //! @param  name    [in] シーンの名前（同じ名前なら置き換える）
        //! @param  factory [in] シーンを作る関数（切り替えのたびに新しく作る）
        //-------------------------------------------------------------
        void RegisterScene(const std::string& name, SceneFactory factory);

        //-------------------------------------------------------------
        // 登録した名前のシーンへの遷移を予約する
        //! @param  name [in] シーンの名前
        //! @return 登録されていて予約できたら true（無い名前なら警告をログへ出して false）
        //! @note   シーンの EventBus へ SceneChangeRequestedEvent を Publish しても同じことになる
        //!         （System など EngineAPI を持たない所から切り替えたいとき用）
        //-------------------------------------------------------------
        bool ChangeScene(const std::string& name);

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
        EngineContext*                 m_context = nullptr;    // エンジンコンテキストを保持
        std::unique_ptr<GameSceneBase> m_currentScene;         // 現在のシーン
        std::unique_ptr<GameSceneBase> m_nextScene;            // 次に遷移予約されたシーン
        std::unordered_map<std::string, SceneFactory> m_factories;    // 名前で切り替えるシーンの作り方
        Tsukino::ECS::ScopedConnection m_changeRequest;        // 今のシーンの SceneChangeRequestedEvent の購読
    };
}    // namespace Tsukino::EngineIntegration

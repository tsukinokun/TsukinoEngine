//-------------------------------------------------------------
//! @file    GameSceneManager.cpp
//! @brief   ゲームシーン管理クラスの実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include "Tsukino/EngineIntegration/Scene/GameSceneManager.hpp"
#include "Tsukino/EngineIntegration/Scene/GameSceneBase.hpp"

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {

    //-------------------------------------------------------------
    //! @brief  デストラクタ
    //-------------------------------------------------------------
    GameSceneManager::~GameSceneManager() {
        // 現在のシーンが存在する場合は終了処理を呼び出す
        if(m_currentScene) {
            m_currentScene->OnExit();
        }
    }

    //-------------------------------------------------------------
    //! @brief  次のシーンへの遷移を予約する
    //-------------------------------------------------------------
    void GameSceneManager::ChangeScene(std::unique_ptr<GameSceneBase> newScene) {
        m_nextScene = std::move(newScene);
    }

    //-------------------------------------------------------------
    //! @brief  シーンの更新
    //-------------------------------------------------------------
    void GameSceneManager::Update(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        // 遷移予約がある場合はシーンを切り替える
        if(m_nextScene) {
            // 現在のシーンの終了処理
            if(m_currentScene) {
                m_currentScene->OnExit();
            }

            // 新しいシーンへ所有権を移動し初期化
            m_currentScene = std::move(m_nextScene);
            if(m_currentScene) {
                m_currentScene->OnInitialize(api);
            }
        }

        // 現在のアクティブなシーンの更新
        if(m_currentScene) {
            m_currentScene->OnUpdate(api, deltaTime);
        }
    }

    //-------------------------------------------------------------
    //! @brief  現在アクティブなシーンを取得する
    //-------------------------------------------------------------
    GameSceneBase* GameSceneManager::GetCurrentScene() const {
        return m_currentScene.get();
    }

}    // namespace Tsukino::EngineIntegration

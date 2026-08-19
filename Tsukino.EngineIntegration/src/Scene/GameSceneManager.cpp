//-------------------------------------------------------------
//! @file    GameSceneManager.cpp
//! @brief   ゲームシーン管理クラスの実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include "Tsukino/EngineIntegration/Scene/GameSceneManager.hpp"
#include "Tsukino/EngineIntegration/Scene/GameSceneBase.hpp"
#include "Tsukino/EngineIntegration/EngineContext.hpp"

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {
    namespace {
        //-------------------------------------------------------------
        //! @brief  シーンが所有するオブジェクトへのポインタをコンテキストから外す
        //! @param  context [in] エンジンコンテキスト（nullptr 可）
        //! @details
        //! EngineContext にはシーンが所有する System の生ポインタが入る
        //! （例: EffectSystem）。シーンを破棄した後もポインタが残っていると、
        //! 次のフレームの Renderer::Render() が解放済みメモリを触る。
        //! シーンを手放す箇所では必ずこの関数を通すこと。
        //-------------------------------------------------------------
        void ClearSceneOwnedPointers(EngineContext* context) {
            if(!context)
                return;

            context->effectSystem  = nullptr;
            context->physicsSystem = nullptr;
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief コンストラクタ
    //-------------------------------------------------------------
    GameSceneManager::GameSceneManager() = default;

    //-------------------------------------------------------------
    //! @brief  デストラクタ
    //-------------------------------------------------------------
    GameSceneManager::~GameSceneManager() {
        // 現在のシーンが存在する場合は終了処理を呼び出す
        if(m_currentScene) {
            m_currentScene->OnExit();
        }

        // シーンを破棄する前に、シーン所有オブジェクトへの参照を切る
        ClearSceneOwnedPointers(m_context);
    }

    //-------------------------------------------------------------
    //! @brief 初期化
    //-------------------------------------------------------------
    void GameSceneManager::Initialize(EngineContext* context) {
        m_context = context;
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

            //-------------------------------------------------------------
            // 旧シーンを解放する前に、旧シーンが所有していたオブジェクトへの
            // コンテキスト上の生ポインタを必ず切る。
            // 新シーンの Initialize() が改めて自身のポインタを登録する。
            //-------------------------------------------------------------
            ClearSceneOwnedPointers(m_context);
            m_currentScene.reset();

            // 新しいシーンへ所有権を移動し初期化
            m_currentScene = std::move(m_nextScene);
            if(m_currentScene) {
                m_currentScene->Initialize(api, m_context);
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

//------------------------------------------------------------
//! @file       EngineAPI.cpp
//! @brief      エンジンからAPIを提供するクラスの実装
//! @author     山﨑愛
//------------------------------------------------------------
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneManager.hpp>

#include <Tsukino/Renderer/Renderer.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Engine/ECS/SystemManager.hpp>

#include <Tsukino/Core/Window.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Audio/AudioManager.hpp>

#include <memory>

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {
    //------------------------------------------------------------
    //! @brief コンストラクタ
    //------------------------------------------------------------
    EngineAPI::EngineAPI(EngineContext& context)
        : m_context(context) {
    }

    //------------------------------------------------------------
    //! @brief シーン遷移関数
    //------------------------------------------------------------
    void EngineAPI::ChangeScene(std::unique_ptr<GameSceneBase> newScene) {
        // GameSceneManagerにシーン遷移を予約する
        if(m_context.gameSceneManager) {
            m_context.gameSceneManager->ChangeScene(std::move(newScene));
        }
    }

    //------------------------------------------------------------
    //! @brief メッセージ処理関数
    //------------------------------------------------------------
    bool EngineAPI::ProcessMessages() {
        return m_context.window->ProcessMessages();
    }

    //------------------------------------------------------------
    // 更新関数
    //------------------------------------------------------------
    void EngineAPI::Update(float deltaTime) {
        // ゲームシーンの更新
        m_context.gameSceneManager->Update(*this, deltaTime);
        // 入力システムの更新
        m_context.inputSystem->Update();
        // オーディオの更新
        if (m_context.audioManager) {
            m_context.audioManager->Update(deltaTime);
        }
    }

    //------------------------------------------------------------
    //! @brief 描画関数
    //------------------------------------------------------------
    void EngineAPI::Render() {
        m_context.renderer->Render();
    }
}    // namespace Tsukino::EngineIntegration

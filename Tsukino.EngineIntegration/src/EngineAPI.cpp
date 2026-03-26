//------------------------------------------------------------
//! @file       EngineAPI.cpp
//! @brief      エンジンからAPIを提供するクラスの実装
//! @author     山﨑愛
//------------------------------------------------------------
#include <Tsukino/EngineIntegration/EngineAPI.hpp>

#include <Tsukino/Renderer/Renderer.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Core/Window.hpp>
#include <Tsukino/Engine/ECS/SystemManager.hpp>

#include <memory>

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {
    //------------------------------------------------------------
    //! @brief コンストラクタ
    //------------------------------------------------------------
    EngineAPI::EngineAPI(EngineContext& context, Tsukino::ECS::Registry& registry)
        : m_context(context)
        , m_registry(registry) {
    }

    //------------------------------------------------------------
    //! @brief メッセージ処理関数
    //------------------------------------------------------------
    bool EngineAPI::ProcessMessages() {
        return m_context.window->ProcessMessages();
    }

    //------------------------------------------------------------
    //! @brief システムの追加関数
    //------------------------------------------------------------
    void EngineAPI::AddSystem(std::shared_ptr<Tsukino::ECS::ISystem> system, int priority) {
        // 内部の SystemManager に委譲する
        m_context.systemManager->AddSystem(std::move(system), priority);
    }

    //------------------------------------------------------------
    // 更新関数
    //------------------------------------------------------------
    void EngineAPI::Update(float deltaTime) {
        m_context.systemManager->Update(m_registry, deltaTime);
    }

    //------------------------------------------------------------
    //! @brief 描画関数
    //------------------------------------------------------------
    void EngineAPI::Render() {
        m_context.renderer->Render();
    }
}    // namespace Tsukino::EngineIntegration

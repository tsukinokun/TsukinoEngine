//-------------------------------------------------------------
//! @file    PenguinGameScene.cpp
//! @brief   ペンギンゲームのサンプルシーンの実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/Scene/PenguinGameScene.hpp>

#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneManager.hpp>

#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Input/KeyCodes.hpp>

#include <Tsukino/Renderer/Renderer.hpp>

#include <entt/entt.hpp>
#include <hlsl++.h>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    void PenguinGameScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) {
        // コンテキストをレジストリから取得
        Tsukino::EngineIntegration::EngineContext* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        //-------------------------------------------------------------
        // イベントバスをレジストリから取得
        //-------------------------------------------------------------
        Tsukino::ECS::EventBus& eventBus = m_scene.GetEventBus();
        //--------------------------------------------------------------
        // クリアカラーを透明に設定
        //--------------------------------------------------------------
        context->renderer->SetClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    }

    void PenguinGameScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        m_scene.Update(deltaTime);
    }

    void PenguinGameScene::OnExit() {
        // シーンの終了処理をここに記述
    }
};    // namespace Tsukino::Sandbox

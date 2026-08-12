//-------------------------------------------------------------
//! @file   ResultInputSystem.cpp
//! @brief  ResultInputSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/ResultInputSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/GameStateComponent.hpp>
#include <Tsukino/Sandbox/Scene/LuckGameSampleScene.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneManager.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Input/KeyCodes.hpp>

#include <memory>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void ResultInputSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        (void)deltaTime;

        auto* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!context || !context->inputSystem || !context->gameSceneManager) {
            return;
        }
        if(!registry.HasContext<GameStateComponent>()) {
            return;
        }

        GameStateComponent& state = registry.GetContext<GameStateComponent>();
        if(state.phase != GamePhase::Result) {
            return;
        }

        if(context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Space)) {
            // シーンを丸ごと読み直して再戦する（個別のリセット処理は行わない）
            context->gameSceneManager->ChangeScene(std::make_unique<Tsukino::Sandbox::LuckGameSampleScene>());
        }
    }

}    // namespace LuckGameSampleScene::ECS

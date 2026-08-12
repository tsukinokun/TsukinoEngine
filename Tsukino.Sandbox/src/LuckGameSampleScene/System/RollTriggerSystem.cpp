//-------------------------------------------------------------
//! @file   RollTriggerSystem.cpp
//! @brief  RollTriggerSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/RollTriggerSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/RoundComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/GameStateComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Util/DiceThrowUtil.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Input/KeyCodes.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void RollTriggerSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        (void)deltaTime;

        auto* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!context || !context->inputSystem) {
            return;
        }
        if(!registry.HasContext<GameStateComponent>()) {
            return;
        }

        GameStateComponent& state = registry.GetContext<GameStateComponent>();
        if(state.player == entt::null || state.cpu == entt::null) {
            return;
        }

        if(!context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Space)) {
            return;
        }

        if(state.phase == GamePhase::Ready) {
            // 開始入力：両者同時にサイコロを投げる
            PlayerComponent& player = registry.GetComponent<PlayerComponent>(state.player);
            PlayerComponent& cpu    = registry.GetComponent<PlayerComponent>(state.cpu);

            RoundComponent& playerRound = registry.GetComponent<RoundComponent>(player.roundEntity);
            RoundComponent& cpuRound    = registry.GetComponent<RoundComponent>(cpu.roundEntity);

            ThrowDiceSet(registry, playerRound);
            ThrowDiceSet(registry, cpuRound);

            player.phase = TurnPhase::Rolling;
            cpu.phase    = TurnPhase::Rolling;
            state.phase  = GamePhase::Rolling;
            return;
        }

        if(state.phase == GamePhase::Rolling) {
            // 目なし/ヒフミで待機中の人間側のみ、再度のスペース入力で振り直す
            // （CPU側は CPURerollSystem がタイマーで自動的に振り直すため、ここでは扱わない）
            PlayerComponent& player = registry.GetComponent<PlayerComponent>(state.player);
            if(player.phase == TurnPhase::Waiting) {
                RoundComponent& playerRound = registry.GetComponent<RoundComponent>(player.roundEntity);
                ThrowDiceSet(registry, playerRound);
                player.phase = TurnPhase::Rolling;
            }
        }
    }

}    // namespace LuckGameSampleScene::ECS

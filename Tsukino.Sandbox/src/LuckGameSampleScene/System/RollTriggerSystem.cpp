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
            // 開始入力：それまで空中で回転しながら待機していた両者のサイコロを、
            // このタイミングで同時に投下する（DropDiceSetはHovering以外のサイコロには
            // 作用しないため、この分岐に万一複数回入っても多重投下にはならない）
            PlayerComponent& player = registry.GetComponent<PlayerComponent>(state.player);
            PlayerComponent& cpu    = registry.GetComponent<PlayerComponent>(state.cpu);

            RoundComponent& playerRound = registry.GetComponent<RoundComponent>(player.roundEntity);
            RoundComponent& cpuRound    = registry.GetComponent<RoundComponent>(cpu.roundEntity);

            (void)DropDiceSet(registry, playerRound);
            (void)DropDiceSet(registry, cpuRound);

            player.phase = TurnPhase::Rolling;
            cpu.phase    = TurnPhase::Rolling;
            state.phase  = GamePhase::Rolling;
            return;
        }

        if(state.phase == GamePhase::Rolling) {
            // 目なし/ヒフミで待機中の人間側のみ、再度のスペース入力で投下する
            // （リスポン後の投下待ち中はDiceRollState::Hoveringなので、DropDiceSetで投下できる。
            // 　CPU側は CPURerollSystem がタイマーで自動的に投下するため、ここでは扱わない）
            PlayerComponent& player = registry.GetComponent<PlayerComponent>(state.player);
            if(player.phase == TurnPhase::Waiting) {
                RoundComponent& playerRound = registry.GetComponent<RoundComponent>(player.roundEntity);
                // リスポン直後でまだHoveringに達していない（Respawning中の）可能性があるため、
                // 実際に投下できた場合のみ手番を進める
                if(DropDiceSet(registry, playerRound)) {
                    player.phase = TurnPhase::Rolling;
                }
            }
        }
    }

}    // namespace LuckGameSampleScene::ECS

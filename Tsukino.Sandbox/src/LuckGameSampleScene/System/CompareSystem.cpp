//-------------------------------------------------------------
//! @file   CompareSystem.cpp
//! @brief  CompareSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/CompareSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/RoundComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/GameStateComponent.hpp>

#include <Tsukino/Core/Log.hpp>

#include <string>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void CompareSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        (void)deltaTime;

        if(!registry.HasContext<GameStateComponent>()) {
            return;
        }

        GameStateComponent& state = registry.GetContext<GameStateComponent>();

        // まだ投げていない、あるいは既に決着している間は何もしない
        if(state.phase != GamePhase::Rolling) {
            return;
        }
        if(state.player == entt::null || state.cpu == entt::null) {
            return;
        }

        PlayerComponent& player = registry.GetComponent<PlayerComponent>(state.player);
        PlayerComponent& cpu    = registry.GetComponent<PlayerComponent>(state.cpu);

        // 両者の手番が確定するまで待つ
        if(player.phase != TurnPhase::Resolved || cpu.phase != TurnPhase::Resolved) {
            return;
        }

        if(player.eliminated && cpu.eliminated) {
            state.outcome = RoundOutcome::Draw;
        } else if(player.eliminated) {
            state.outcome = RoundOutcome::CpuWin;
        } else if(cpu.eliminated) {
            state.outcome = RoundOutcome::PlayerWin;
        } else {
            RoundComponent& playerRound = registry.GetComponent<RoundComponent>(player.roundEntity);
            RoundComponent& cpuRound    = registry.GetComponent<RoundComponent>(cpu.roundEntity);

            if(playerRound.kind != cpuRound.kind) {
                state.outcome = (playerRound.kind > cpuRound.kind) ? RoundOutcome::PlayerWin : RoundOutcome::CpuWin;
            } else if(playerRound.subValue != cpuRound.subValue) {
                state.outcome = (playerRound.subValue > cpuRound.subValue) ? RoundOutcome::PlayerWin : RoundOutcome::CpuWin;
            } else {
                state.outcome = RoundOutcome::Draw;
            }
        }

        state.phase = GamePhase::Result;

#ifdef _DEBUG
        Tsukino::Core::Log::Info("[LuckGameSampleScene] Round resolved. outcome=" + std::to_string(static_cast<int>(state.outcome)));
#endif
    }

}    // namespace LuckGameSampleScene::ECS

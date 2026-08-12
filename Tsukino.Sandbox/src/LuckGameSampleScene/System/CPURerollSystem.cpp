//-------------------------------------------------------------
//! @file   CPURerollSystem.cpp
//! @brief  CPURerollSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/CPURerollSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/RoundComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/CPUControllerComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Util/DiceThrowUtil.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void CPURerollSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto view = registry.View<PlayerComponent, CPUControllerComponent>();

        view.each([&](entt::entity, PlayerComponent& player, CPUControllerComponent& cpuController) {
            // タイマーが動いていない（＝振り直し待ちではない）間は何もしない
            if(cpuController.rerollDelayTimer <= 0.0f) {
                return;
            }

            cpuController.rerollDelayTimer -= deltaTime;
            if(cpuController.rerollDelayTimer > 0.0f) {
                return;    // まだ「考え中」
            }

            cpuController.rerollDelayTimer = 0.0f;

            RoundComponent& round = registry.GetComponent<RoundComponent>(player.roundEntity);
            ThrowDiceSet(registry, round);
            player.phase = TurnPhase::Rolling;
        });
    }

}    // namespace LuckGameSampleScene::ECS

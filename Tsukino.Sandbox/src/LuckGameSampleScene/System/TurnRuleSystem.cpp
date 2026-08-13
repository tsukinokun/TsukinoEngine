//-------------------------------------------------------------
//! @file   TurnRuleSystem.cpp
//! @brief  TurnRuleSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/TurnRuleSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/RoundComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/CPUControllerComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Util/DiceThrowUtil.hpp>

#include <Tsukino/Core/Log.hpp>

#include <cstdlib>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {
    namespace {
        constexpr u8 kMaxRollCount = 3;    //!< これに達したら即敗北確定

        constexpr float kCpuRerollDelayMin = 0.6f;    //!< CPUの「考え中」演出の最小秒数
        constexpr float kCpuRerollDelayMax = 1.2f;    //!< CPUの「考え中」演出の最大秒数

        //-------------------------------------------------------------
        //! @brief  kCpuRerollDelayMin 〜 kCpuRerollDelayMax のランダムな遅延秒数を返す
        //-------------------------------------------------------------
        float RandomRerollDelay() {
            const float t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            return kCpuRerollDelayMin + t * (kCpuRerollDelayMax - kCpuRerollDelayMin);
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void TurnRuleSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        (void)deltaTime;

        auto view = registry.View<PlayerComponent>();

        view.each([&](entt::entity playerEntity, PlayerComponent& player) {
            // 投げた直後〜静止判定待ち以外は対象外（役が確定するまでは何もしない）
            if(player.phase != TurnPhase::Rolling) {
                return;
            }

            RoundComponent& round = registry.GetComponent<RoundComponent>(player.roundEntity);
            if(!round.judged) {
                return;
            }

            const bool isNoHand = (round.kind == Hand::MeNashi || round.kind == Hand::HiFuMi);

            if(!isNoHand) {
                // 役が確定したので、このプレイヤーの手番は終了
                player.phase = TurnPhase::Resolved;
                return;
            }

            player.rollCount++;

            if(player.rollCount >= kMaxRollCount) {
                // 3回とも役なしだったので即敗北確定
                player.eliminated = true;
                player.phase      = TurnPhase::Resolved;

#ifdef _DEBUG
                Tsukino::Core::Log::Info("[LuckGameSampleScene] Player eliminated (3 failed rolls).");
#endif
                return;
            }

            // 役なし・再挑戦可能なので、着地した位置から揺すり直すのではなく、
            // お椀中心上空の投下待ち位置へリスポンさせる
            RespawnDiceSet(registry, round);
            player.phase = TurnPhase::Waiting;

            if(CPUControllerComponent* cpuController = registry.try_get<CPUControllerComponent>(playerEntity)) {
                // CPU側は「考え中」を挟んでCPURerollSystemが自動で振り直す
                cpuController->rerollDelayTimer = RandomRerollDelay();
            }
            // 人間側はここでWaitingに戻すだけで、再度のスペース入力（RollTriggerSystem）を待つ
        });
    }

}    // namespace LuckGameSampleScene::ECS

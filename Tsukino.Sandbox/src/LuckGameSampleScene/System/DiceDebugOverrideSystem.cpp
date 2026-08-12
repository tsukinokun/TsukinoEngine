//-------------------------------------------------------------
//! @file   DiceDebugOverrideSystem.cpp
//! @brief  DiceDebugOverrideSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/DiceDebugOverrideSystem.hpp>
#ifdef _DEBUG
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/RoundComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/DiceComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Log.hpp>

#include <array>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {
    namespace {
        //-------------------------------------------------------------
        //! @brief  RoundComponentが持つ3つのサイコロへ強制的に出目を書き込み、
        //!         役の再判定が走るよう judged をリセットする
        //-------------------------------------------------------------
        void ForceFaceValues(Tsukino::ECS::Registry& registry, RoundComponent& round, std::array<u8, 3> values) {
            for(int i = 0; i < 3; ++i) {
                DiceComponent& dice = registry.GetComponent<DiceComponent>(round.dice[i]);
                dice.state          = DiceRollState::Settled;
                dice.confirmedValue = values[i];
                dice.confirmed      = true;
            }
            round.judged = false;    // HandJudgeSystemに再判定させる
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void DiceDebugOverrideSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        (void)deltaTime;

        auto* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!context || !context->inputSystem) {
            return;
        }

        auto& input = *context->inputSystem;

        // フェーズ4以降はRoundComponentが2つ（プレイヤー/CPU）存在するため、
        // キー入力時は両方に同じ強制出目を適用する
        auto view = registry.View<RoundComponent>();

        view.each([&](entt::entity, RoundComponent& round) {
            if(input.IsKeyPressed(Tsukino::Input::KeyCode::D1)) {
                ForceFaceValues(registry, round, {1, 1, 1});    // ピンゾロ
                Tsukino::Core::Log::Info("[LuckGameSampleScene][Debug] Force: PinZoro");
            } else if(input.IsKeyPressed(Tsukino::Input::KeyCode::D2)) {
                ForceFaceValues(registry, round, {4, 4, 4});    // アラシ
                Tsukino::Core::Log::Info("[LuckGameSampleScene][Debug] Force: Arashi");
            } else if(input.IsKeyPressed(Tsukino::Input::KeyCode::D3)) {
                ForceFaceValues(registry, round, {4, 5, 6});    // シゴロ
                Tsukino::Core::Log::Info("[LuckGameSampleScene][Debug] Force: Shigoro");
            } else if(input.IsKeyPressed(Tsukino::Input::KeyCode::D4)) {
                ForceFaceValues(registry, round, {1, 2, 3});    // ヒフミ
                Tsukino::Core::Log::Info("[LuckGameSampleScene][Debug] Force: HiFuMi");
            } else if(input.IsKeyPressed(Tsukino::Input::KeyCode::D5)) {
                ForceFaceValues(registry, round, {2, 2, 5});    // 目
                Tsukino::Core::Log::Info("[LuckGameSampleScene][Debug] Force: Me");
            } else if(input.IsKeyPressed(Tsukino::Input::KeyCode::D6)) {
                ForceFaceValues(registry, round, {1, 2, 4});    // 目なし
                Tsukino::Core::Log::Info("[LuckGameSampleScene][Debug] Force: MeNashi");
            }
        });
    }

}    // namespace LuckGameSampleScene::ECS
#endif    // _DEBUG

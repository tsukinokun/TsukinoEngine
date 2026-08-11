//-------------------------------------------------------------
//! @file   HandJudgeSystem.cpp
//! @brief  HandJudgeSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/HandJudgeSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/RoundComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/DiceComponent.hpp>

#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <array>
#include <string>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {
    namespace {
        //-------------------------------------------------------------
        //! @brief  3つの出目から役を判定する
        //! @param  values [in]  3つの出目（1〜6）
        //! @param  outSub [out] Meの場合の目、Arashi/PinZoroのゾロ目の値
        //! @return 判定された役
        //-------------------------------------------------------------
        Hand JudgeHand(std::array<u8, 3> values, u8& outSub) {
            std::sort(values.begin(), values.end());
            const u8 a = values[0];
            const u8 b = values[1];
            const u8 c = values[2];

            if(a == b && b == c && a == 1) {
                outSub = 1;
                return Hand::PinZoro;
            }
            if(a == b && b == c) {
                outSub = a;
                return Hand::Arashi;
            }
            if(a == 1 && b == 2 && c == 3) {
                return Hand::HiFuMi;
            }
            if(a == 4 && b == 5 && c == 6) {
                return Hand::Shigoro;
            }
            if(a == b) {
                outSub = c;
                return Hand::Me;
            }
            if(b == c) {
                outSub = a;
                return Hand::Me;
            }
            return Hand::MeNashi;
        }

#ifdef _DEBUG
        //-------------------------------------------------------------
        //! @brief  デバッグログ用に役名を文字列化する
        //-------------------------------------------------------------
        std::string HandToDebugString(Hand hand, u8 subValue) {
            switch(hand) {
            case Hand::PinZoro:
                return "PinZoro";
            case Hand::Arashi:
                return "Arashi(" + std::to_string(subValue) + ")";
            case Hand::Shigoro:
                return "Shigoro";
            case Hand::Me:
                return "Me(" + std::to_string(subValue) + ")";
            case Hand::HiFuMi:
                return "HiFuMi";
            case Hand::MeNashi:
                return "MeNashi";
            default:
                return "None";
            }
        }
#endif
    }    // namespace

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void HandJudgeSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        (void)deltaTime;

        auto view = registry.View<RoundComponent>();

        view.each([&](entt::entity, RoundComponent& round) {
            // 既に判定済みなら何もしない（次の投げ直しでリセットされるまで再判定しない）
            if(round.judged) {
                return;
            }

            DiceComponent& dice0 = registry.GetComponent<DiceComponent>(round.dice[0]);
            DiceComponent& dice1 = registry.GetComponent<DiceComponent>(round.dice[1]);
            DiceComponent& dice2 = registry.GetComponent<DiceComponent>(round.dice[2]);

            // 3つ全ての出目が確定するまで待つ
            if(!dice0.confirmed || !dice1.confirmed || !dice2.confirmed) {
                return;
            }

            u8   subValue = 0;
            Hand hand     = JudgeHand({dice0.confirmedValue, dice1.confirmedValue, dice2.confirmedValue}, subValue);

            round.kind     = hand;
            round.subValue = subValue;
            round.judged   = true;

#ifdef _DEBUG
            Tsukino::Core::Log::Info("[LuckGameSampleScene] Hand judged: " + HandToDebugString(hand, subValue) + " (" + std::to_string(dice0.confirmedValue)
                                     + "," + std::to_string(dice1.confirmedValue) + "," + std::to_string(dice2.confirmedValue) + ")");
#endif
        });
    }

}    // namespace LuckGameSampleScene::ECS

//-------------------------------------------------------------
//! @file   TurnRuleSystem.hpp
//! @brief  TurnRuleSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @class  TurnRuleSystem
    //! @brief  役判定後、目なし/ヒフミ（役なし）の再挑戦と3回失敗による敗北確定を扱うシステム。
    //!         人間側は即座に振り直し入力待ちへ、CPU側は「考え中」を挟んで自動再挑戦する。
    //!         優先度は HandJudgeSystem より後に登録すること。
    //-------------------------------------------------------------
    class TurnRuleSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace LuckGameSampleScene::ECS

//-------------------------------------------------------------
//! @file   RollTriggerSystem.hpp
//! @brief  RollTriggerSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @class  RollTriggerSystem
    //! @brief  人間の入力を検知するシステム。
    //!         ・GamePhase::Ready中のスペース入力で両者に同時にサイコロを投げさせる
    //!         ・目なし/ヒフミで待機中(TurnPhase::Waiting)の人間側は、再度のスペース入力で
    //!           自分のサイコロだけを投げ直す
    //-------------------------------------------------------------
    class RollTriggerSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace LuckGameSampleScene::ECS

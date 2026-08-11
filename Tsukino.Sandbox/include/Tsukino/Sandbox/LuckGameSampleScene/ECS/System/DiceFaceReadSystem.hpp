//-------------------------------------------------------------
//! @file   DiceFaceReadSystem.hpp
//! @brief  DiceFaceReadSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @class  DiceFaceReadSystem
    //! @brief  静止したサイコロの上向きの面を判定し、出目を確定させるシステム
    //! @details DiceComponent::state が Settled になったサイコロについて、
    //!          各面のローカル法線をワールド回転で変換し、重力方向（+Y）との
    //!          内積が最大の面を「上を向いている面」として出目を確定する。
    //!          DiceRestDetectionSystem より後に登録すること。
    //-------------------------------------------------------------
    class DiceFaceReadSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace LuckGameSampleScene::ECS

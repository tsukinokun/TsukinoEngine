//-------------------------------------------------------------
//! @file   DiceHoverLimitSystem.hpp
//! @brief  DiceHoverLimitSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @class  DiceHoverLimitSystem
    //! @brief  投下待ち（Hovering）中のサイコロの角速度に上限をかけるシステム。
    //! @details SetupDiceHoverはその場で回転させ続けるため一定のトルクを
    //!          与え続けており、投下待ちが長引くほど角速度が際限なく増え続けてしまう
    //!          （待機時間に比例して投下時の勢いが強くなり、着地が乱れて出目の
    //!          誤判定につながっていた）。毎フレーム角速度の大きさを確認し、
    //!          上限を超えていたら超過分を打ち消す角力積を与えて頭打ちにする。
    //!          優先度は PhysicsSystem の直後に登録すること。
    //-------------------------------------------------------------
    class DiceHoverLimitSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace LuckGameSampleScene::ECS

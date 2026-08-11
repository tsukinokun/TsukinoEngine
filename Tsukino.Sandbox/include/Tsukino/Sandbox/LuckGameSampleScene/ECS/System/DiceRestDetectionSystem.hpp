//-------------------------------------------------------------
//! @file   DiceRestDetectionSystem.hpp
//! @brief  DiceRestDetectionSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @class  DiceRestDetectionSystem
    //! @brief  サイコロの速度・角速度を監視し、静止したかどうかを判定するシステム
    //! @details RigidbodyComponentの線速度・角速度が一定時間しきい値を下回り続けたら
    //!          DiceComponent::state を Settled に遷移させる。
    //!          優先度は PhysicsSystem（コリジョン/剛体更新）より後に登録すること。
    //-------------------------------------------------------------
    class DiceRestDetectionSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace LuckGameSampleScene::ECS

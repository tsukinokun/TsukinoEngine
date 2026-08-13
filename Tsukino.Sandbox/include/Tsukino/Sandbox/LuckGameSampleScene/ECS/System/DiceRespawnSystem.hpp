//-------------------------------------------------------------
//! @file   DiceRespawnSystem.hpp
//! @brief  DiceRespawnSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @class  DiceRespawnSystem
    //! @brief  RespawnDiceSetでKinematic化・テレポートされたサイコロ
    //!         （DiceRollState::Respawning）を、PhysicsSystemがテレポートを
    //!         反映した後にDynamic＋位置フリーズ（Hovering）へ引き継ぐシステム。
    //!         PhysicsSystemより後段（優先度13）に置くことで、
    //!         「テレポート要求を出したのと同じフレーム」で誤って
    //!         Dynamicに戻してしまうことを防いでいる。
    //-------------------------------------------------------------
    class DiceRespawnSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace LuckGameSampleScene::ECS

//-------------------------------------------------------------
//! @file   PickupSystem.hpp
//! @brief  PickupSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @class  PickupSystem
    //! @brief  落ちているアイテム（PickupComponent）とプレイヤーの距離判定、
    //!         対象のハイライト演出、「Fキーで拾う」UIの更新、
    //!         Fキー押下時の取得処理をまとめて行うシステム
    //-------------------------------------------------------------
    class PickupSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //! @param registry  [in] エンジンのECSレジストリのラッパー
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace ActionGame::ECS

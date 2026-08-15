//-------------------------------------------------------------
//! @file   EnemySystem.hpp
//! @brief  EnemySystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @class  EnemySystem
    //! @brief  敵の簡易AI（プレイヤーを検知して追跡する）を行うシステム
    //-------------------------------------------------------------
    class EnemySystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //! @param registry  [in] エンジンのECSレジストリのラッパー
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace ActionGame::ECS

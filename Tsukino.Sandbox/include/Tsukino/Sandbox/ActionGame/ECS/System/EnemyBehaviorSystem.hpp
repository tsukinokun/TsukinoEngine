//-------------------------------------------------------------
//! @file   EnemyBehaviorSystem.hpp
//! @brief  EnemyBehaviorSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @class  EnemyBehaviorSystem
    //! @brief  BehaviorTreeComponentを持つ敵の黒板を更新し、ビヘイビアツリーを進行させるシステム。
    //!         対象はEnemySystem（従来の直進追跡のみ）とは排他（BehaviorTreeComponentの有無で分岐）
    //-------------------------------------------------------------
    class EnemyBehaviorSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //! @param registry  [in] エンジンのECSレジストリのラッパー
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace ActionGame::ECS

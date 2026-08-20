//-------------------------------------------------------------
//! @file   BigZombieBehavior.hpp
//! @brief  BigZombie用ビヘイビアツリー構築関数の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Sandbox/ActionGame/ECS/Component/BehaviorTreeComponent.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @brief  BigZombie用のビヘイビアツリーを構築する
    //! @details
    //! Selector（記憶あり）
    //!  ├─ Sequence "Attack"  : 攻撃射程内 かつ クールタイム明け なら攻撃を再生する
    //!  ├─ Sequence "Chase"   : 索敵範囲内ならプレイヤーへ近づく
    //!  └─ Action   "Idle"    : どちらも成立しなければ待機する
    //! @return 構築済みツリーのルートノード
    //-------------------------------------------------------------
    std::shared_ptr<EnemyBehaviorNode> BuildBigZombieTree();
}    // namespace ActionGame::ECS

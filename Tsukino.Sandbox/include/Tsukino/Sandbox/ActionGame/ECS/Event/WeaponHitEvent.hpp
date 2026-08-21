//-------------------------------------------------------------
//! @file   WeaponHitEvent.hpp
//! @brief  武器が敵にヒットした際のイベント
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

#include <entt/entt.hpp>
#include <hlsl++.h>

// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {

    //-------------------------------------------------------------
    //! @struct WeaponHitEvent
    //! @brief  ヒットストップ・エフェクト・SE等の副作用処理に使う通知イベント
    //-------------------------------------------------------------
    struct WeaponHitEvent {
        entt::entity   attacker;      //!< 攻撃した側のエンティティ（武器の所有者）
        entt::entity   weapon;        //!< ヒットした武器エンティティ
        entt::entity   target;        //!< ヒットを受けた敵エンティティ
        hlslpp::float3 hitPosition;   //!< ヒット位置（ワールド空間。演出用）
        float          damage;        //!< 実際に与えたダメージ量
    };

}    // namespace ActionGame::ECS

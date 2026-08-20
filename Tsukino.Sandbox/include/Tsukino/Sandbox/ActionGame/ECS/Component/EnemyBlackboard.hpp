//-------------------------------------------------------------
//! @file   EnemyBlackboard.hpp
//! @brief  EnemyBlackboard構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <hlsl++.h>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @struct EnemyBlackboard
    //! @brief  敵のビヘイビアツリーが参照する黒板データ。
    //!         EnemyBehaviorSystemが毎フレーム更新し、ツリーのCondition/Actionノードが読み書きする
    //-------------------------------------------------------------
    struct EnemyBlackboard {
        Tsukino::ECS::Entity playerEntity     = entt::null;               //!< 検知対象のプレイヤー
        hlslpp::float3        playerPosition{0.0f, 0.0f, 0.0f};            //!< プレイヤーのワールド位置
        float                  distanceToPlayer = 0.0f;                     //!< プレイヤーとの水平（XZ）距離
        bool                   hasPlayer        = false;                    //!< プレイヤーが存在するか（未生成/破棄済みならfalse）
    };
}    // namespace ActionGame::ECS

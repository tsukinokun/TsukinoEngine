//-------------------------------------------------------------
//! @file   PlayerMovementComponent.hpp
//! @brief  プレイヤー（ボール）の移動制御コンポーネント
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

#include <entt/entt.hpp>

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    //-------------------------------------------------------------
    //! @struct PlayerMovementComponent
    //! @brief  カメラ基準での矢印キー移動を行うための設定
    //-------------------------------------------------------------
    struct PlayerMovementComponent {
        entt::entity cameraEntity = entt::null;    //!< 向きの基準にするゲームカメラのエンティティ

        float moveForce = 200.0f;    //!< 1秒あたりに加える力の大きさ（質量・摩擦に応じて要調整）
        float torqueForce = 800.0f;    //!< 転がり回転用のトルク係数
    };

}    // namespace WaterGame::ECS

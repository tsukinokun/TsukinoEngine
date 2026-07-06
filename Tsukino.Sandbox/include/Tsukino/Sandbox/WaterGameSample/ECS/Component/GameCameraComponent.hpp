//-------------------------------------------------------------
//! @file   GameCameraComponent.hpp
//! @brief  ゲームカメラコンポーネントの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

#include <Tsukino/Core/ECS/Entity/Entity.hpp>

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    //-------------------------------------------------------------
    //! @struct GameCameraComponent
    //! @brief  対象entityの周囲を回転しながら追従するカメラの状態を保持する
    //-------------------------------------------------------------
    struct GameCameraComponent {
        Tsukino::ECS::Entity target = entt::null;    //!< 追従対象のエンティティ（ボールなど）

        float distance = 200.0f;    //!< ターゲットからの水平距離
        float height   = 200.0f;    //!< ターゲットからの基準高さオフセット

        float yaw   = 0.0f;    //!< 水平回転角（ラジアン）：A/Dで変化
        float pitch = 0.4f;    //!< 上下回転角（ラジアン）：W/Sで変化

        float minPitch = 0.15f;    //!< pitchの下限（真横に近すぎないように）
        float maxPitch = 1.30f;    //!< pitchの上限（真上に近すぎないように）

        float rotateSpeed = 1.5f;    //!< 回転速度（ラジアン/秒）

        float followLerpSpeed = 10.0f;    //!< 追従の補間速度（大きいほど瞬時に追従）
    };

}    // namespace WaterGame::ECS

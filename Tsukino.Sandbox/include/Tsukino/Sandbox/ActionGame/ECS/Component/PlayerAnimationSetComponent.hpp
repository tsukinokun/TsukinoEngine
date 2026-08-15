//-------------------------------------------------------------
//! @file   PlayerAnimationSetComponent.hpp
//! @brief  PlayerAnimationSetComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @enum   PlayerAnimState
    //! @brief  プレイヤーのアニメーションステート
    //-------------------------------------------------------------
    enum class PlayerAnimState {
        Idle,
        Run,
        FastRun,
        Jump,
    };

    //-------------------------------------------------------------
    //! @struct PlayerAnimationSetComponent
    //! @brief  プレイヤーのステートマシンが参照するアニメーションクリップ一式と、
    //!         現在のステート（PlayerAnimationSystemが管理）を保持するコンポーネント
    //-------------------------------------------------------------
    struct PlayerAnimationSetComponent {
        Tsukino::Asset::AssetHandle idleClip;       //!< 待機
        Tsukino::Asset::AssetHandle runClip;         //!< 通常移動
        Tsukino::Asset::AssetHandle fastRunClip;    //!< スプリント移動
        Tsukino::Asset::AssetHandle jumpClip;        //!< ジャンプ

        PlayerAnimState currentState = PlayerAnimState::Idle;    //!< 現在のステート（クリップの重複要求を避けるため保持）
    };
}    // namespace ActionGame::ECS

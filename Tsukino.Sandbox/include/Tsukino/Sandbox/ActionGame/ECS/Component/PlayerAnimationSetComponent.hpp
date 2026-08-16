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
        Attack,
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
        Tsukino::Asset::AssetHandle attackClip;      //!< 攻撃（単発。将来的にコンボへ拡張予定）

        PlayerAnimState currentState = PlayerAnimState::Idle;    //!< 現在のステート（クリップの重複要求を避けるため保持）

        //-------------------------------------------------------------
        // 攻撃アニメーションの終了判定は、原則としてAnimationPlayerComponent::is_finished
        // （実クリップの再生完了）で行う。attackTimerはAttack突入からの経過時間を数える
        // 保険用のウォッチドッグで、attackClipの設定ミス等でis_finishedが立たなかった場合に
        // Attackステートへ無限に留まり続けるのを防ぐためだけに使う（通常プレイでは発火しない想定）
        //-------------------------------------------------------------
        float attackTimer          = 0.0f;    //!< Attackステートに入ってからの経過時間
        float attackTimeoutSafety = 2.0f;    //!< attackTimerがこの秒数を超えたら強制的にAttackステートを抜ける保険値
    };
}    // namespace ActionGame::ECS

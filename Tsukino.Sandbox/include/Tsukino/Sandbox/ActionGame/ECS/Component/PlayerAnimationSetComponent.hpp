//-------------------------------------------------------------
//! @file   PlayerAnimationSetComponent.hpp
//! @brief  PlayerAnimationSetComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Core/typedef.hpp>
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
        Attack1,    //!< 連撃1段目
        Attack2,    //!< 連撃2段目
        Attack3,    //!< 連撃3段目
    };

    //-------------------------------------------------------------
    //! @struct AttackStep
    //! @brief  1本のFBXクリップから切り出した「連撃1段分」の定義。
    //!         Weapon Attack.fbxは3回斬るモーションが1クリップに入っているため、
    //!         同じクリップハンドルを時間レンジだけ変えて3回参照する
    //-------------------------------------------------------------
    struct AttackStep {
        Tsukino::Asset::AssetHandle clip;
        u32   animationIndex   = 1;      //!< Mixamo製FBXはindex 0が1tickのスタブ、index 1が実モーション
        float startTime        = 0.0f;   //!< クリップ内の開始時刻（秒）
        float endTime          = 0.0f;   //!< クリップ内の終了時刻（秒）
        float comboWindowStart = 0.5f;   //!< 現在未使用（将来のチューニング用に残置）。連撃は段の再生完了後にのみ進行する
        float hitWindowDuration = -1.0f; //!< この段だけの当たり判定有効時間（秒）。-1でWeaponComponent::activeDurationを使う
        float fadeTime         = 0.05f;  //!< 段へ入るときのクロスフェード時間（素早く反応させるため短め）
        float playbackSpeed    = 1.0f;   //!< この段の再生速度倍率（1.0が等速。大きいほど振りが速くなり、次段への移行も早まる）
        bool  inPlace          = true;   //!< 攻撃モーションのルート前進を殺す（移動はCharacterControllerが担当するため）
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

        static constexpr u32 kAttackComboCount = 3;    //!< 連撃の段数
        AttackStep            attackSteps[kAttackComboCount];    //!< 各段の再生範囲

        PlayerAnimState currentState = PlayerAnimState::Idle;    //!< 現在のステート（クリップの重複要求を避けるため保持）

        u32   attackComboIndex    = 0;        //!< 現在再生中の段（0..kAttackComboCount-1）
        bool  attackInputBuffered = false;    //!< 先行入力を1回だけ保持する（連打しても2段先へは飛ばない）

        //-------------------------------------------------------------
        // 攻撃アニメーションの終了判定は、原則としてAnimationPlayerComponent::is_finished
        // （実クリップの再生完了）で行う。attackTimerは現在の段に突入してからの経過時間を数える
        // 保険用のウォッチドッグで、クリップ設定ミス等でis_finishedが立たなかった場合に
        // 攻撃ステートへ無限に留まり続けるのを防ぐためだけに使う（通常プレイでは発火しない想定）。
        // 1段の長さ（約1.18秒）に対して十分な余裕を持たせた値にしている
        //-------------------------------------------------------------
        float attackTimer          = 0.0f;    //!< 現在の攻撃段に入ってからの経過時間
        float attackTimeoutSafety = 2.5f;    //!< attackTimerがこの秒数を超えたら強制的に攻撃ステートを抜ける保険値
    };
}    // namespace ActionGame::ECS

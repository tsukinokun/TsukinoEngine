//-------------------------------------------------------------
//! @file   EnemyAnimationSetComponent.hpp
//! @brief  EnemyAnimationSetComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Core/typedef.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @enum   EnemyAnimState
    //! @brief  ビヘイビアツリー駆動の敵のアニメーションステート
    //-------------------------------------------------------------
    enum class EnemyAnimState {
        Idle,      //!< 待機（Idleクリップが無いため、その場の足踏みで代用する）
        Walk,      //!< 追跡移動
        Attack,    //!< 攻撃
    };

    //-------------------------------------------------------------
    //! @struct EnemyAnimationSetComponent
    //! @brief  EnemyBehaviorSystemが決めた行動と、EnemyAnimationSystemが管理する
    //!         実際のアニメーションステートを橋渡しするコンポーネント
    //-------------------------------------------------------------
    struct EnemyAnimationSetComponent {
        Tsukino::Asset::AssetHandle walkClip;      //!< Mutant Walking（Idleもin_place再生でこれを流用する）
        Tsukino::Asset::AssetHandle attackClip;    //!< Mutant Swiping
        u32   animationIndex = 1;                    //!< Mixamo製FBXはindex 0が1tickのスタブ、index 1が実モーション

        EnemyAnimState currentState = EnemyAnimState::Idle;    //!< 現在のステート（EnemyAnimationSystemが管理）
        EnemyAnimState desiredState = EnemyAnimState::Idle;    //!< 今フレームであるべきステート（EnemyBehaviorSystemが書き込む）

        //-------------------------------------------------------------
        // 攻撃アニメーションの終了判定は、原則としてAnimationPlayerComponent::is_finished
        // （実クリップの再生完了）で行う。attackTimerはAttackへ入ってからの経過時間を数える
        // 保険用のウォッチドッグで、クリップ設定ミス等でis_finishedが立たなかった場合に
        // 攻撃ステートへ無限に留まり続けるのを防ぐためだけに使う（PlayerAnimationSetComponentと同じ考え方）
        //-------------------------------------------------------------
        float attackTimer          = 0.0f;    //!< Attackへ入ってからの経過時間
        float attackTimeoutSafety = 3.0f;    //!< attackTimerがこの秒数を超えたら強制的に攻撃ステートを抜ける保険値
    };
}    // namespace ActionGame::ECS

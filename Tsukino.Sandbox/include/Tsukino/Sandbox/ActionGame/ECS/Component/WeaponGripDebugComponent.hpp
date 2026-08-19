//-------------------------------------------------------------
//! @file   WeaponGripDebugComponent.hpp
//! @brief  WeaponGripDebugComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @enum   WeaponGripEditTarget
    //! @brief  WeaponGripDebugSystemが現在調整しているWeaponComponentのパラメータ
    //-------------------------------------------------------------
    enum class WeaponGripEditTarget {
        GripPoint,        //!< WeaponComponent::gripPointLocal（握り点。姿勢決定後に位置を引く基準）
        AttackLocalOffset,//!< WeaponComponent::attackLocalOffset（手ボーンローカル空間での握り位置オフセット）
        GripRotation,      //!< WeaponComponent::attackGripRotationOffset（攻撃中の握り角度オフセット）
    };

    //-------------------------------------------------------------
    //! @struct WeaponGripDebugComponent
    //! @brief  「Fキーで拾う」UIラベルと同様に1つだけ生成して使い回すHUD用エンティティの目印。
    //!         WeaponGripDebugSystemが調整モードの状態（ON/OFF、編集対象、ステップ幅、ポーズ固定）を
    //!         ここに保持し、毎フレームHUDのFontComponent::textへ書き出す
    //!         （_DEBUGビルドのみ生成・使用される）
    //-------------------------------------------------------------
    struct WeaponGripDebugComponent {
        bool enabled    = false;    //!< 調整モードが有効か（F6でトグル）。無効時はHUD非表示・調整キーも無視する
        bool poseFrozen = false;    //!< 攻撃ポーズを一時固定中か（F10でトグル）。固定中はプレイヤーのアニメーション再生を止め、武器をisAttacking扱いにする

        WeaponGripEditTarget editTarget = WeaponGripEditTarget::GripPoint;    //!< 現在の編集対象（F7で切り替える）

        int stepIndex = 1;    //!< 移動量・回転量の刻み幅インデックス（F8で切り替える。0=小, 1=中, 2=大）
    };
}    // namespace ActionGame::ECS

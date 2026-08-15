//-------------------------------------------------------------
//! @file   WeaponComponent.hpp
//! @brief  WeaponComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <hlsl++.h>

#include <string>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @struct WeaponComponent
    //! @brief  武器エンティティに付与するコンポーネント。
    //!         所有者（プレイヤー）の手ボーンにアタッチして追従しつつ、攻撃入力時に範囲内の敵へダメージを与える。
    //!         ボーンが解決できない場合は所有者のルートTransformからの固定オフセット追従にフォールバックする。
    //!         当たり判定はJolt物理を使わず距離判定で簡易的に行う（Phase Bで本実装に差し替え予定）
    //-------------------------------------------------------------
    struct WeaponComponent {
        Tsukino::ECS::Entity owner = entt::null;    //!< 武器を所持しているエンティティ

        std::string handBoneName      = "mixamorig:RightHand";    //!< アタッチ対象ボーン名
        u32          handBoneNodeIndex = UINT32_MAX;                //!< 解決済みノードindex（未解決/見つからない場合はUINT32_MAX）
        bool         boneResolved      = false;                     //!< ボーン名の解決を試みたか（一度だけ解決するためのフラグ）

        hlslpp::float3     localOffset         = hlslpp::float3(30.0f, 100.0f, 60.0f);    //!< アタッチボーンのローカル空間での握り位置オフセット（ボーン未解決時はルートTransformからのオフセットとして使われる）
        hlslpp::quaternion gripRotationOffset = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    //!< 握り方調整用の追加回転
        float               handTrackingWeight = 1.0f;    //!< 手ボーン位置への追従度（0=ルート位置に留まる, 1=手ボーンに完全追従）。
                                                            //!< アニメーションクリップの腕の振り幅が大きく武器が体から離れすぎる場合に下げて使う

        //-------------------------------------------------------------
        // 「手に持つ」のではなく所有者の周りをふわふわ浮遊させる演出用パラメータ。
        // localOffsetを浮遊位置として使い、その上に上下・左右のゆったりした漂いを加える
        // （旋回はしない。姿勢はgripRotationOffsetを基準にわずかに揺れるだけ）
        //-------------------------------------------------------------
        bool  floatEnabled        = false;    //!< trueで浮遊演出（ふわふわ漂う動き）を有効化する
        float floatBobAmplitude  = 6.0f;      //!< 上下に漂う振れ幅
        float floatBobSpeed      = 1.6f;      //!< 上下の漂いの角速度（rad/sec）
        float floatDriftAmplitude = 4.0f;      //!< 左右・前後に漂う振れ幅（上下と別周期でゆっくり揺れて円を描くように漂う）
        float floatDriftSpeed    = 0.9f;      //!< 左右・前後の漂いの角速度（rad/sec）
        float floatSwayAngle      = 0.12f;     //!< 姿勢が前後に傾く最大角度（ラジアン）。小さく保つことで縦向きをほぼ維持する
        float floatSwaySpeed      = 1.1f;      //!< 姿勢の揺れの角速度（rad/sec）
        float floatTime            = 0.0f;      //!< 浮遊演出用の経過時間（CombatSystemが毎フレーム加算する）

        float damage         = 20.0f;    //!< 命中時に与えるダメージ
        float range           = 90.0f;   //!< 攻撃判定の到達距離（武器位置からの単純な距離判定に使用）
        float activeDuration = 0.25f;    //!< 攻撃入力後、当たり判定が有効な時間（秒）
        float cooldown       = 0.4f;     //!< 攻撃後、再攻撃可能になるまでのクールダウン（秒）

        bool  attackRequested = false;    //!< 攻撃入力を受け取ったか（PlayerSystemがセットする）
        bool  isActive        = false;    //!< 現在当たり判定が有効か
        float activeTimer     = 0.0f;     //!< 当たり判定有効時間の残り
        float cooldownTimer   = 0.0f;     //!< クールダウンの残り
    };
}    // namespace ActionGame::ECS

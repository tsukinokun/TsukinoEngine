//-------------------------------------------------------------
//! @file   WeaponComponent.hpp
//! @brief  WeaponComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

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

        //-------------------------------------------------------------
        // ボーン解決は「どのクリップのnode配列に対して行ったか」をキャッシュし、
        // 再生中のクリップが変わるたびに再解決する（Idle.fbxとAttack.fbx等、別アセットの
        // クリップ間でnode配列のインデックスが一致するとは限らないため、一度解決したら
        // 使い回すのではなく、クリップが変わったら都度解決し直す必要がある）
        //-------------------------------------------------------------
        Tsukino::Asset::AssetHandle resolvedAgainstClip;    //!< 最後にボーン解決を行った時点のクリップ（比較して再解決要否を判定する）

        hlslpp::float3     localOffset         = hlslpp::float3(30.0f, 100.0f, 60.0f);    //!< アタッチボーンのローカル空間での握り位置オフセット（ボーン未解決時はルートTransformからのオフセットとして使われる）
        hlslpp::quaternion gripRotationOffset = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    //!< 握り方調整用の追加回転
        float               handTrackingWeight = 1.0f;    //!< 手ボーン位置への追従度（0=ルート位置に留まる, 1=手ボーンに完全追従）。
                                                            //!< アニメーションクリップの腕の振り幅が大きく武器が体から離れすぎる場合に下げて使う

        //-------------------------------------------------------------
        // 攻撃中（isAttacking=true）はlocalOffset/gripRotationOffsetの代わりにこちらを使う。
        // localOffsetは「ほぼ静止した基準点からの浮遊位置」として調整された値（170ユニット近く離れている）で、
        // 実際に振られる手ボーンにそのまま適用するとテコの原理で大きく・速く振り回されてしまう
        // （振った時に武器が暴れる不具合の原因）。攻撃時は手のひら付近の小さいオフセットを別途用意する
        //-------------------------------------------------------------
        float               attackHandTrackingWeight  = 1.0f;    //!< 攻撃中に代わりに使う追従度。振りの動きにしっかり追従させるため通常1.0
        hlslpp::float3     attackLocalOffset          = hlslpp::float3(0.0f, 0.0f, 0.0f);    //!< 攻撃中の握り位置オフセット（手ボーンローカル空間）。実機で見た目を確認しながら調整する
        hlslpp::quaternion attackGripRotationOffset  = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    //!< 攻撃中の握り角度オフセット。実機で見た目を確認しながら調整する

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

        bool  isAttacking = false;    //!< 攻撃アニメーション再生中か（PlayerAnimationSystemが毎フレームセットする）。
                                       //!< trueの間はfloatEnabledによる浮遊演出を止め、attackHandTrackingWeightで手に追従させる

        //-------------------------------------------------------------
        // isAttackingの真偽値が瞬時に切り替わっても、握りオフセット・回転（浮遊姿勢↔手ボーン追従）が
        // 目標として不連続にジャンプしないよう、0(非攻撃)↔1(攻撃)へ連続的に遷移させるブレンド値。
        // 末尾の指数減衰補間（attachPositionLerpSpeed等）は「目標へ滑らかに近づく」役割であって
        // 「目標自体の飛び」は吸収しきれないため、目標を作る側で先に連続化する
        // （攻撃開始/終了の瞬間に武器がカクッとスナップして見える不具合の原因だった）
        //-------------------------------------------------------------
        float attackBlend      = 0.0f;     //!< 現在の攻撃ブレンド値（0=非攻撃の浮遊姿勢, 1=攻撃中の手ボーン追従）。CombatSystemが毎フレーム更新する
        float attackBlendSpeed = 15.0f;    //!< attackBlendがisAttackingの目標値へ遷移する速さ（大きいほど素早く切り替わる）

        //-------------------------------------------------------------
        // 攻撃モーションへの出入りやフォールバック切り替えでオフセットが瞬時に変わっても
        // 武器が瞬間移動しないよう、目標位置・姿勢へ指数減衰で追従させる際の速度
        // （大きいほど素早く吸い付く。PlayerComponent::turnLerpSpeedと同じ考え方）
        //-------------------------------------------------------------
        float attachPositionLerpSpeed = 18.0f;    //!< 目標位置への追従速度
        float attachRotationLerpSpeed = 18.0f;    //!< 目標姿勢への追従速度
    };
}    // namespace ActionGame::ECS

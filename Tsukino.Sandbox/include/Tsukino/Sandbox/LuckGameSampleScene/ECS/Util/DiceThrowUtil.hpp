//-------------------------------------------------------------
//! @file   DiceThrowUtil.hpp
//! @brief  サイコロの投擲・リセット処理の共通関数の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <hlsl++.h>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    struct RoundComponent;

    //-------------------------------------------------------------
    //! @brief  お椀中心から見た、diceIndex(0〜2)番目のサイコロの投下待ち位置（相対座標）を返す
    //!         （3個がお椀の中に収まるよう、初期生成とRespawnDiceSetの両方でこれを使う）
    //! @param  diceIndex [in] RoundComponent::dice内でのインデックス（0〜2）
    //-------------------------------------------------------------
    hlslpp::float3 ComputeDiceSpawnOffset(int diceIndex);

    //-------------------------------------------------------------
    //! @brief  1つのサイコロをお椀中心上空へ再配置する（Dynamicボディはトランスフォームへの
    //!         直接書き込みだけでは動かせないため、速度を打ち消した上でお椀中心へ向かう
    //!         補正インパルスを与えることで擬似的にテレポートさせる）
    //! @param  registry     [in] ECSレジストリ
    //! @param  diceEntity   [in] 対象のサイコロエンティティ（RigidbodyComponent必須）
    //! @param  bowlCenter   [in] 戻す先のお椀中心のワールド座標
    //-------------------------------------------------------------
    void RepositionDiceAboveBowl(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity diceEntity, const hlslpp::float3& bowlCenter);

    //-------------------------------------------------------------
    //! @brief  1つのサイコロを「投下待ち（空中で静止＋回転）」状態にする
    //!         （位置を全軸フリーズして重力の影響を無効化しつつ、トルクを与えて
    //!         その場で回転させ続ける。DropDiceSetで投下されるまでこの状態が続く）
    //! @param  registry   [in]     ECSレジストリ
    //! @param  diceEntity [in]     対象のサイコロエンティティ（RigidbodyComponent/DiceComponent必須）
    //-------------------------------------------------------------
    void SetupDiceHover(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity diceEntity);

    //-------------------------------------------------------------
    //! @brief  RoundComponentが束ねる3つのサイコロを投下する（空中で回転待機している
    //!         サイコロの位置フリーズを解除し、重力とその時点の角速度に任せて落とす）
    //!         DiceRollState::Hoveringのサイコロのみを対象とするため、
    //!         誤って複数回呼び出されても2回目以降は何もしない（多重投下の防止）
    //! @param  registry [in]     ECSレジストリ
    //! @param  round    [inout]  投下する対象のRoundComponent
    //! @return 1個でも実際に投下できたらtrue（リスポン直後でまだHoveringに達していない
    //!         場合など、全てHovering以外だった場合はfalse。呼び出し側はfalseの場合、
    //!         手番を先へ進めてはいけない）
    //-------------------------------------------------------------
    [[nodiscard]] bool DropDiceSet(Tsukino::ECS::Registry& registry, RoundComponent& round);

    //-------------------------------------------------------------
    //! @brief  RoundComponentが束ねる3つのサイコロを、お椀中心上空の投下待ち位置へ
    //!         「リスポン」させる（目なし/ヒフミでの振り直し時に使用）。
    //!         現在の位置・姿勢に関わらず、まずKinematic化して1〜2フレームかけて
    //!         正しい位置へテレポートさせ、その後DiceRespawnSystemが自動的に
    //!         SetupDiceHoverと同じ「空中で静止＋回転」状態へ引き継ぐ。
    //!         其の場での揺すり直しと違い、既存の位置・速度を一切引き継がないため、
    //!         着地時に散らかった状態から再スタートしても暴れない。
    //! @param  registry [in]     ECSレジストリ
    //! @param  round    [inout]  リスポンさせる対象のRoundComponent
    //-------------------------------------------------------------
    void RespawnDiceSet(Tsukino::ECS::Registry& registry, RoundComponent& round);

}    // namespace LuckGameSampleScene::ECS

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
    //! @brief  1つのサイコロをお椀中心上空へ再配置する（Dynamicボディはトランスフォームへの
    //!         直接書き込みだけでは動かせないため、速度を打ち消した上でお椀中心へ向かう
    //!         補正インパルスを与えることで擬似的にテレポートさせる）
    //! @param  registry     [in] ECSレジストリ
    //! @param  diceEntity   [in] 対象のサイコロエンティティ（RigidbodyComponent必須）
    //! @param  bowlCenter   [in] 戻す先のお椀中心のワールド座標
    //-------------------------------------------------------------
    void RepositionDiceAboveBowl(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity diceEntity, const hlslpp::float3& bowlCenter);

    //-------------------------------------------------------------
    //! @brief  RoundComponentが束ねる3つのサイコロを投げ直す
    //!         （お椀中心上空へ再配置してからインパルスを与え、判定関連のフラグをリセットする）
    //! @param  registry [in]     ECSレジストリ
    //! @param  round    [inout]  投げ直す対象のRoundComponent
    //-------------------------------------------------------------
    void ThrowDiceSet(Tsukino::ECS::Registry& registry, RoundComponent& round);

    //-------------------------------------------------------------
    //! @brief  RoundComponentが束ねる3つのサイコロを、投げ直さずに待機状態へ戻す
    //!         （目なし/ヒフミで人間側が振り直し入力を待つ場合に使用）
    //! @param  registry [in]     ECSレジストリ
    //! @param  round    [inout]  待機状態へ戻す対象のRoundComponent
    //-------------------------------------------------------------
    void ResetRoundToIdle(Tsukino::ECS::Registry& registry, RoundComponent& round);

}    // namespace LuckGameSampleScene::ECS

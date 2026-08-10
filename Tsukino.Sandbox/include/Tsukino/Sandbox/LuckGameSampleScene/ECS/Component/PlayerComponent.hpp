//-------------------------------------------------------------
//! @file   PlayerComponent.hpp
//! @brief  PlayerComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <entt/entt.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @enum class TurnPhase
    //! @brief  1プレイヤーの手番の進行状態
    //-------------------------------------------------------------
    enum class TurnPhase {
        Waiting,     //!< 入力/振り直しタイミング待ち
        Rolling,     //!< 投げた直後〜静止判定待ち
        Resolved     //!< 役が確定した（または3回失敗で敗北確定した）
    };

    //-------------------------------------------------------------
    //! @struct PlayerComponent
    //! @brief  プレイヤー（人間・CPU共通）の進行状態
    //-------------------------------------------------------------
    struct PlayerComponent {
        TurnPhase             phase       = TurnPhase::Waiting;
        u8                     rollCount   = 0;              //!< 目なし/ヒフミでの振り直し回数（3で即敗北）
        bool                   eliminated  = false;           //!< 3回役なしによる敗北が確定したか
        Tsukino::ECS::Entity   roundEntity = entt::null;      //!< このプレイヤーに紐づくRoundComponentのエンティティ
    };

}    // namespace LuckGameSampleScene::ECS

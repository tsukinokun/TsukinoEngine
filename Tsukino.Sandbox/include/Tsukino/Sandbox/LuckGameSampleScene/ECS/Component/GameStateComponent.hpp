//-------------------------------------------------------------
//! @file   GameStateComponent.hpp
//! @brief  GameStateComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <entt/entt.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @enum class GamePhase
    //! @brief  ゲーム全体の進行フェーズ
    //-------------------------------------------------------------
    enum class GamePhase {
        Ready,      //!< 開始入力待ち
        Rolling,    //!< 投げた直後〜両者の決着待ち
        Compare,    //!< 両者確定後、勝敗を比較する
        Result      //!< 勝敗確定・表示中（スペースキーでシーン再読込待ち）
    };

    //-------------------------------------------------------------
    //! @enum class RoundOutcome
    //! @brief  ラウンドの勝敗結果
    //-------------------------------------------------------------
    enum class RoundOutcome {
        None,
        PlayerWin,
        CpuWin,
        Draw
    };

    //-------------------------------------------------------------
    //! @struct GameStateComponent
    //! @brief  ゲーム全体の進行状態。registry.SetContext<GameStateComponent>()で
    //!         シングルトンとして登録して使用する
    //-------------------------------------------------------------
    struct GameStateComponent {
        GamePhase             phase   = GamePhase::Ready;
        RoundOutcome            outcome = RoundOutcome::None;
        Tsukino::ECS::Entity    player  = entt::null;    //!< 人間プレイヤーのPlayerComponentエンティティ
        Tsukino::ECS::Entity    cpu     = entt::null;    //!< CPUプレイヤーのPlayerComponentエンティティ
    };

}    // namespace LuckGameSampleScene::ECS

//-------------------------------------------------------------
//! @file   EnemyComponent.hpp
//! @brief  EnemyComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @struct EnemyComponent
    //! @brief  敵エンティティであることを表すコンポーネント。
    //!         プレイヤー・武器との当たり判定はJolt物理を使わず距離判定で簡易的に行う
    //!         （Phase Bで物理形状ベースの判定に差し替え予定）
    //-------------------------------------------------------------
    struct EnemyComponent {
        float moveSpeed      = 120.0f;    //!< プレイヤーを追跡する速度（1ユニット≒1cm規約）
        float detectRange    = 600.0f;    //!< プレイヤーを追跡し始める距離
        float bodyRadius     = 40.0f;     //!< 簡易的な当たり判定半径（武器のヒット判定・プレイヤー接触判定の両方に使用）
        float contactDamage  = 10.0f;     //!< プレイヤーに接触した際に与えるダメージ
        float attackInterval = 1.0f;      //!< 接触ダメージの再発生までのクールタイム（秒）
        float attackTimer    = 0.0f;      //!< クールタイムの残り
    };
}    // namespace ActionGame::ECS

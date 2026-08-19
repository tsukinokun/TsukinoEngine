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
    //!         武器のヒット判定はJolt物理のカプセルセンサー（CollisionComponent、spawnEnemyが付与）で行う。
    //!         プレイヤーとの接触ダメージ判定は引き続きTransform間の距離判定で簡易的に行う
    //-------------------------------------------------------------
    struct EnemyComponent {
        float moveSpeed      = 120.0f;    //!< プレイヤーを追跡する速度（1ユニット≒1cm規約）
        float detectRange    = 600.0f;    //!< プレイヤーを追跡し始める距離
        float bodyRadius     = 40.0f;     //!< 簡易的な当たり判定半径（プレイヤー接触判定に使用。武器ヒット判定はCollisionComponentのカプセルを使う）
        float contactDamage  = 10.0f;     //!< プレイヤーに接触した際に与えるダメージ
        float attackInterval = 1.0f;      //!< 接触ダメージの再発生までのクールタイム（秒）
        float attackTimer    = 0.0f;      //!< クールタイムの残り
    };
}    // namespace ActionGame::ECS

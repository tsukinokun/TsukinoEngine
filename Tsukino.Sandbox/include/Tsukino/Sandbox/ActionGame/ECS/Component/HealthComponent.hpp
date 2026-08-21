//-------------------------------------------------------------
//! @file   HealthComponent.hpp
//! @brief  HealthComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @struct HealthComponent
    //! @brief  HP（体力）を持つエンティティに付与するコンポーネント
    //-------------------------------------------------------------
    struct HealthComponent {
        float maxHealth     = 100.0f;    //!< 最大HP
        float currentHealth = 100.0f;    //!< 現在HP
        bool  isDead         = false;    //!< HPが尽きたか（実際のエンティティ破棄は各Systemが行う）

        // --- 頭上HPバー（HealthBarSystemが使用。無い場合はentt::nullのまま） ---
        Tsukino::ECS::Entity hpBarBackgroundEntity = entt::null;    //!< HPバー背景エンティティ
        Tsukino::ECS::Entity hpBarFillEntity       = entt::null;    //!< HPバー残量エンティティ
        float                hpBarVisibleTimer     = 0.0f;          //!< 0より大きい間だけHPバーを表示する残り時間（秒）。被弾時にCombatSystemがセットする
    };
}    // namespace ActionGame::ECS

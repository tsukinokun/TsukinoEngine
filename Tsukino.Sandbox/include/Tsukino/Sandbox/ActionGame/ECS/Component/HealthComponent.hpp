//-------------------------------------------------------------
//! @file   HealthComponent.hpp
//! @brief  HealthComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
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
    };
}    // namespace ActionGame::ECS

//-------------------------------------------------------------
//! @file   PlayerMovementSystem.hpp
//! @brief  PlayerMovementSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    //-------------------------------------------------------------
    //! @class  PlayerMovementSystem
    //! @brief  カメラの向きを基準に矢印キーでボール（Dynamic Rigidbody）を動かすシステム
    //-------------------------------------------------------------
    class PlayerMovementSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace WaterGame::ECS

//-------------------------------------------------------------
//! @file   PenguinAnimationSystem.hpp
//! @brief  PenguinAnimationSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : PenguinGame::ECS
namespace PenguinGame::ECS {
    //-------------------------------------------------------------
    //! @class  PenguinAnimationSystem
    //! @brief  ペンギンのアニメーションのシステム
    //-------------------------------------------------------------
    class PenguinAnimationSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //! @param registry  [in] エンジンのECSレジストリのラッパー
        //! @param deltaTime [in] デルタタイム
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace PenguinGame::ECS

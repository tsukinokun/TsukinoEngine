//-------------------------------------------------------------
//! @file   HealthBarSystem.hpp
//! @brief  HealthBarSystemクラスの宣言
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @class  HealthBarSystem
    //! @brief  HealthComponentの残量を、頭上HPバー（背景・残量の2エンティティ、
    //!         WorldAnchorComponentで敵の頭上に追従）の見た目へ反映するシステム。
    //!         被弾直後のみ表示し、一定時間経過で自動的に非表示へ戻す
    //-------------------------------------------------------------
    class HealthBarSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace ActionGame::ECS

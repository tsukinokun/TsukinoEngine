//-------------------------------------------------------------
//! @file   PenguinAnimatorComponent.hpp
//! @brief  PenguinAnimatorComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <Tsukino/Sandbox/PenguinGame/Data/PenguinSkinDefinition.hpp>
// 名前空間 : PenguinGame::ECS
namespace PenguinGame::ECS {
    //-------------------------------------------------------------
    //! @struct PenguinAnimatorComponent
    //! @brief  ペンギンのアニメーションに必要なComponent
    //-------------------------------------------------------------
    struct PenguinAnimatorComponent {
        Tsukino::ECS::Entity centerEntity;
        Tsukino::ECS::Entity leftHandEntity;
        Tsukino::ECS::Entity rightHandEntity;

        // 現在適用されているスキン定義へのポインタ/ハンドル
        // これを差し替えるだけで、全てのパーツが一瞬で切り替わる
        const PenguinGame::Data::PenguinSkinDefinition* currentSkin = nullptr;

        // --- アニメーション状態 ---
        float blinkTimer    = 0.0f;
        float blinkInterval = 3.0f;

        bool isCenterFrame2 = false;
        bool isLeftFrame2   = false;
        bool isRightFrame2  = false;
    };
}    // namespace PenguinGame::ECS

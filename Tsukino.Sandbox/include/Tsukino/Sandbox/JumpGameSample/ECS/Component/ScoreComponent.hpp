//-------------------------------------------------------------
//! @file   ScoreComponent.hpp
//! @brief  ScoreComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
    //-------------------------------------------------------------
    //! @struct ScoreComponent
    //! @brief  スコア管理エンティティに必要なComponent
    //-------------------------------------------------------------
    struct ScoreComponent {
        int value;    // スコアの値
    };
}    // namespace JumpGameSample::ECS

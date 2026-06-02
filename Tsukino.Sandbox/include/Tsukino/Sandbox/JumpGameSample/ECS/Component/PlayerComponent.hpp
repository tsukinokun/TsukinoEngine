//-------------------------------------------------------------
//! @file   PlayerComponent.hpp
//! @brief  PlayerComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
    //-------------------------------------------------------------
    //! @struct PlayerComponent
    //! @brief  プレイヤーに必要なComponent
    //-------------------------------------------------------------
    struct PlayerComponent {
        bool dummy = false;    //!< とりあえずダミーのフラグ
    };
}    // namespace JumpGameSample::ECS

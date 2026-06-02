//-------------------------------------------------------------
//! @file   PlatformComponent.hpp
//! @brief  PlatformComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
    //-------------------------------------------------------------
    //! @struct PlatformComponent
    //! @brief  土台に必要なComponent
    //-------------------------------------------------------------
    struct PlatformComponent {
        float speed    = 50.0f;
        bool  isMoving = true;
    };
}    // namespace JumpGameSample::ECS

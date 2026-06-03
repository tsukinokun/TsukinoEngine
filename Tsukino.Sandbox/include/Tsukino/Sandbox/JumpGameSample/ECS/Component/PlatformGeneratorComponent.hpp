//-------------------------------------------------------------
//! @file   PlatformGeneratorComponent.hpp
//! @brief  PlatformGeneratorComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
    //-------------------------------------------------------------
    //! @struct PlatformGeneratorComponent
    //! @brief  土台生成器に必要なComponent
    //-------------------------------------------------------------
    struct PlatformGeneratorComponent {
        float spawnDistance = 0.0f;    // 次の土台が出るまでの距離やオフセットなど
    };
}    // namespace JumpGameSample::ECS

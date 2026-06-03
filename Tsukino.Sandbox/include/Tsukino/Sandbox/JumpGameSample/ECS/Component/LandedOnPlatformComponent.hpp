//-------------------------------------------------------------
//! @file   LandedOnPlatformComponent.hpp
//! @brief  LandedOnPlatformComponentクラスの宣言
//! @brief  LandedOnPlatformComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
    //-------------------------------------------------------------
    //! @struct LandedOnPlatformComponent
    //! @brief  どの土台に乗っているかを示すComponent
    //-------------------------------------------------------------
    struct LandedOnPlatformComponent {
        Tsukino::ECS::Entity platformEntity = entt::null;
    };

}    // namespace JumpGameSample::ECS

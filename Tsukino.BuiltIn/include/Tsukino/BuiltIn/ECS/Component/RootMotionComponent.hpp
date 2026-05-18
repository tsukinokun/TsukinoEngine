//-------------------------------------------------------------
//! @file   RootMotionComponent.hpp
//! @brief  RootMotionComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  RootMotionComponent
    //! @brief  ルートモーションの移動量と回転量を管理するコンポーネント
    //-------------------------------------------------------------
    struct RootMotionComponent {
        hlslpp::float3     delta_position = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 前フレームからの移動量
        hlslpp::quaternion delta_rotation = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 前フレームからの回転量
    };
}    // namespace Tsukino::BuiltIn::ECS

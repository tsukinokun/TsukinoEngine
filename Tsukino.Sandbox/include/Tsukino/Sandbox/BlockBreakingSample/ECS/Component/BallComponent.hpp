//-------------------------------------------------------------
//! @file   BallComponent.hpp
//! @brief  BallComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
// 名前空間 : BlockBreakingSample::ECS
namespace BlockBreakingSample::ECS {
    //-------------------------------------------------------------
    //! @struct BallComponent
    //! @brief  ボールに必要なCmoponent
    //-------------------------------------------------------------
    struct BallComponent {
        bool           IsLaunched = false;                                //!< 発射されたかどうかのフラグ
        float          speed      = 200.0f;                               //!< ボールの移動速度
        hlslpp::float3 offset     = hlslpp::float3(0.0f, 40.0f, 0.0f);    // 発射前のパドルからの相対位置
    };
}    // namespace BlockBreakingSample::ECS

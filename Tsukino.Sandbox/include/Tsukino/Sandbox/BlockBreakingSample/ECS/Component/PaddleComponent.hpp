//-------------------------------------------------------------
//! @file   PaddleComponent.hpp
//! @brief  PaddleComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
// 名前空間 : BlockBreakingSample::ECS
namespace BlockBreakingSample::ECS {
    //-------------------------------------------------------------
    //! @struct PaddleComponent
    //! @brief  パドルに必要なCmoponent
    //-------------------------------------------------------------
    struct PaddleComponent {
        float speed = 150.0f;    //!< パドルの移動速度
        // 横の移動制限を設ける
        float clampRange = 150.0f;    //!< パドルの移動範囲の半分
    };
}    // namespace BlockBreakingSample::ECS

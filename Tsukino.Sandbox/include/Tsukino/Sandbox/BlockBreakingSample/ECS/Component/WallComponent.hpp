//-------------------------------------------------------------
//! @file   WallComponent.hpp
//! @brief  WallComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
// 名前空間 : BlockBreakingSample::ECS
namespace BlockBreakingSample::ECS {
    //-------------------------------------------------------------
    //! @struct WallComponent
    //! @brief  壁に必要なCmoponent
    //-------------------------------------------------------------
    struct WallComponent {
        bool dummy = true;    // これを追加して構造体のサイズを1バイト以上にする
    };
}    // namespace BlockBreakingSample::ECS

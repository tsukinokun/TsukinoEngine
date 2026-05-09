//-------------------------------------------------------------
//! @file   BrickComponent.hpp
//! @brief  BrickComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
// 名前空間 : BlockBreakingSample::ECS
namespace BlockBreakingSample::ECS {
    //-------------------------------------------------------------
    //! @struct BrickComponent
    //! @brief  壁に必要なCmoponent
    //-------------------------------------------------------------
    struct BrickComponent {
        bool dead = false;    // ブロックが壊れたかどうか、当たり判定の後にこれをtrueにして、システム側でEntityを消す
    };
}    // namespace BlockBreakingSample::ECS

//-------------------------------------------------------------
//! @file   SkeletonOutputComponent.hpp
//! @brief  SkeletonOutputComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  SkeletonOutputComponent
    //! @brief  スケルトンの出力を管理するコンポーネント
    //-------------------------------------------------------------
    struct SkeletonOutputComponent {
        static constexpr int MAX_BONES = 128;                // 最大ボーン数
        alignas(16) float local_matrices[MAX_BONES][16];    // 各ボーンのローカル行列
        u32 bone_count;                                     // 実際のボーン数
    };

}    // namespace Tsukino::BuiltIn::ECS

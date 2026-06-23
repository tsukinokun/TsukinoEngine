//-------------------------------------------------------------
//! @file   DraggableComponent.hpp
//! @brief  DraggableComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @brief  ドラッグ可能なエンティティのためのコンポーネント
    //-------------------------------------------------------------
    struct DraggableComponent {
        // ドラッグ中かどうかのフラグ
        bool isDragging = false;
        // ドラッグ開始時に「マウス位置 - スプライト位置」をここに保存する
        hlslpp::float2 dragOffset = {0.0f, 0.0f};
    };
}    // namespace Tsukino::BuiltIn::ECS

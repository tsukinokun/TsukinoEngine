//-------------------------------------------------------------
//! @file   UIClipComponent.hpp
//! @brief  UIClipComponentクラスの宣言
//-------------------------------------------------------------
#pragma once
#include <hlsl++.h>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct UIClipComponent
    //! @brief  画面UIの切り取り枠
    //! @note   このエンティティの子孫（TransformComponent::parent を辿って行き着くもの）の
    //!         画面空間のスプライト（SpriteSpace::Screen）と文字は、枠の外が描かれず、
    //!         枠の外ではマウスの当たり判定（PointerTargetComponent）も効かない。
    //!         枠の中心はこのエンティティのワールド位置（スプライトと同じく中心基準）、
    //!         大きさは size にワールドの拡大率を掛けたもの。
    //!         入れ子にした場合は、一番近い祖先の枠だけを使う
    //-------------------------------------------------------------
    struct UIClipComponent {
        hlslpp::float2 size = hlslpp::float2(0.0f, 0.0f);    // 枠の大きさ（画面ピクセル）
    };
}    // namespace Tsukino::BuiltIn::ECS

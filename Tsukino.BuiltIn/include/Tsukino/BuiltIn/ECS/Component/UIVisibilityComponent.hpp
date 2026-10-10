//-------------------------------------------------------------
//! @file   UIVisibilityComponent.hpp
//! @brief  UIVisibilityComponentクラスの宣言
//-------------------------------------------------------------
#pragma once

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct UIVisibilityComponent
    //! @brief  画面UIの表示・非表示（子孫ごと切り替える）
    //! @note   visible が false の間、このエンティティと子孫（TransformComponent::parent を辿って行き着くもの）の
    //!         画面空間のスプライト（SpriteSpace::Screen）・文字・画面に描く 3D モデル（ScreenModelComponent）は描かれず、
    //!         マウスの当たり判定（PointerTargetComponent）も効かず、スクロール（ScrollViewComponent）も受け付けない。
    //!         タブの中身やウィンドウなど、まとまった UI をまるごと出し入れするのに使う
    //!         （部品のスケールや文字を書き換えて隠す必要がない。親に付けて切り替えるだけ）。
    //!         入れ子にした場合は、祖先のどれか1つでも false なら隠れる
    //-------------------------------------------------------------
    struct UIVisibilityComponent {
        bool visible = true;    // false なら自身と子孫を隠す
    };
}    // namespace Tsukino::BuiltIn::ECS

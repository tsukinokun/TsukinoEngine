//----------------------------------------------------------------------------
//! @file   UICanvasGroupComponent.hpp
//! @brief  画面UIを子孫ごとまとめて透明にするコンポーネント
//----------------------------------------------------------------------------
#pragma once

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 画面UIを子孫ごとまとめて透明にします。
    //! @note   このエンティティと子孫（TransformComponent::parent を辿って行き着くもの）の
    //!         画面空間のスプライト・文字（縁取りも）・画面に描く 3D モデル（ScreenModelComponent。anchor の祖先も見る）の
    //!         α に alpha を掛ける。入れ子にした場合は、祖先の値をすべて掛け合わせる。
    //!         合計がほぼ 0 の間は、マウスの当たり判定（PointerTargetComponent）も効かない。
    //!         パネルのフェードなど、まとまった UI の透明度を、部品の色を書き換えずに変えるのに使う
    struct UICanvasGroupComponent {
        float alpha = 1.0f;    // 不透明度（0 で見えない、1 でそのまま）
    };
}    // namespace Tsukino::BuiltIn::ECS

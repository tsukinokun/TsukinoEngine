//----------------------------------------------------------------------------
//! @file   UIToggleComponent.hpp
//! @brief  画面UIの切り替え（押すたびに入・切、または組の中から1つを選ぶ）のコンポーネント
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/EntityRef/EntityRef.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 画面UIの切り替えです。入・切と、押した入力だけを持ち、見た目は持ちません。
    //! @note   PointerTargetComponent と一緒に付ける。UIToggleSystem がクリックを読む。
    //!         group が未設定なら、押すたびに isOn が入れ替わる。
    //!         group に UIToggleGroupComponent を持つエンティティを指すと、同じ group の切り替えのうち
    //!         押したものだけが入になる（ラジオボタン。入のものを押しても切にはならない）。
    //!         選んだ色や弾みなどの見た目は、作る側が isOn と changed を読んで変える
    struct UIToggleComponent {
        Tsukino::ECS::EntityRef group;                  // 属する組（未設定なら単独で入・切する）
        bool                    interactable = true;    // 押せる状態か

        bool isOn    = false;    // 入か（作る側が初期値を書いてよい）
        bool changed = false;    // このフレームに押されて isOn が変わった（UIToggleSystem が書く）
    };

    //! 切り替えの組の印です。
    //! @note   組の中で1つだけを入にする。どのエンティティに付けてもよい（並べた切り替えの親に付けると分かりやすい）
    struct UIToggleGroupComponent {
        bool changed = false;    // このフレームに組の中で選ばれているものが変わった（UIToggleSystem が書く）
    };
}    // namespace Tsukino::BuiltIn::ECS

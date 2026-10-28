//----------------------------------------------------------------------------
//! @file   UISliderComponent.hpp
//! @brief  画面UIのスライダー（つまみを左右に引いて値を決める）のコンポーネント
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/EntityRef/EntityRef.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 画面UIのスライダーです。値と、つかんで動かす入力だけを持ち、見た目は持ちません。
    //! @note   PointerTargetComponent と一緒に、当たりを受けるエンティティへ付ける。
    //!         UISliderSystem が、当たりを押したらつかみ、左ボタンを離すまでマウスの位置から値を決める。
    //!         値は track のローカル座標の X で測る（原点が最小値、trackLength が最大値。
    //!         track が傾いたり揺れたりしていても、その向きに沿って測る）。
    //!         線やつまみの見た目は、作る側が value を読んで動かす
    //!         （塗りの線なら SpriteComponent::fillMode の Horizontal と fillAmount が使える）
    struct UISliderComponent {
        //--------------------------------------------------------------
        // 設定（作る側が決める）
        //--------------------------------------------------------------
        Tsukino::ECS::EntityRef track;                     // 値を測る線の始点（ローカル座標の原点が最小値）。未設定ならこのエンティティ自身
        float                   trackLength     = 100.0f;  // 最小値から最大値までの長さ（track のローカル座標）
        float                   minValue        = 0.0f;    // 最小値
        float                   maxValue        = 1.0f;    // 最大値
        float                   changeThreshold = 0.0f;    // これより小さい変化は無視する（マウスの細かい揺れで値が変わり続けないように）
        bool                    interactable    = true;    // つかめる状態か

        //--------------------------------------------------------------
        // 状態（UISliderSystem が書く。value は作る側が初期値を書いてよい）
        //--------------------------------------------------------------
        float value       = 0.0f;     // 今の値（minValue 〜 maxValue）
        bool  dragging    = false;    // つかんでいる間 true
        bool  dragStarted = false;    // このフレームにつかんだ
        bool  dragEnded   = false;    // このフレームに離した
        bool  changed     = false;    // このフレームに値が変わった
    };
}    // namespace Tsukino::BuiltIn::ECS

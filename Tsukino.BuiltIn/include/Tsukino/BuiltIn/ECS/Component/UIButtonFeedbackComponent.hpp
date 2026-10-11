//----------------------------------------------------------------------------
//! @file   UIButtonFeedbackComponent.hpp
//! @brief  画面UIのボタンの手ごたえ（重なると少し大きく・押すと縮む・クリックで弾む）のコンポーネント
//----------------------------------------------------------------------------
#pragma once

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 画面UIのボタンの手ごたえです。
    //! @note   PointerTargetComponent と一緒に付ける。UITweenSystem が hovered・pressed・clicked を読み、
    //!         UIMotionComponent の拡大率に掛ける（自分の位置を中心に拡大する）。
    //!         interactable が false の間は大きくも小さくもならない（押せないボタン向け）。
    //!         ラベルなど別エンティティも一緒に動かすには、そちらに UIMotionComponent::source でこのボタンを指定する
    struct UIButtonFeedbackComponent {
        //--------------------------------------------------------------
        // 設定（作る側が決める）
        //--------------------------------------------------------------
        float hoverScale   = 1.05f;    // カーソルが重なっている間の拡大率
        float pressScale   = 0.94f;    // 押している間の拡大率
        float clickPunch   = 0.12f;    // クリックしたときに弾む強さ（拡大率に足す最大の量。0 で弾まない）
        float punchSeconds = 0.3f;     // 弾む長さ（秒）
        float speed        = 20.0f;    // 拡大率が目標に近づく速さ（1秒あたりの減衰率）
        bool  interactable = true;     // 押せる状態か（false なら反応しない）

        //--------------------------------------------------------------
        // 状態（UITweenSystem が書く）
        //--------------------------------------------------------------
        float currentScale = 1.0f;     // 今の拡大率（弾みを除く）
        float punchElapsed = -1.0f;    // 弾み始めてからの時間（秒。負なら弾んでいない）
    };
}    // namespace Tsukino::BuiltIn::ECS

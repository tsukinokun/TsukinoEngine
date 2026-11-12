//----------------------------------------------------------------------------
//! @file   InputEvent.hpp
//! @brief  ウィンドウから届く入力の出来事
//! @detail OS ごとの入力（SDL のイベントや Windows のフック）を、エンジン共通の形に
//!         そろえたものです。Window が作り、InputSystem と TextInputSystem へ渡します。
//----------------------------------------------------------------------------
#pragma once

#include <Tsukino/Core/Input/KeyCodes.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <string>

// 名前空間 : Tsukino::Input
namespace Tsukino::Input {
    //! 入力の出来事の種類です。
    enum class InputEventType : u8 {
        KeyDown,        // キーまたはマウスボタンを押した（key に入る）
        KeyUp,          // キーまたはマウスボタンを離した（key に入る）
        MouseMove,      // マウスが動いた（x, y に入る）
        MouseWheel,     // ホイールを回した（wheel に入る。1 で 1 段）
        TextInput,      // 文字が確定した（text に UTF-8 で入る）
        TextEditing,    // IME で変換中の文字列が変わった（text に UTF-8 で入る。空なら変換の終わり）
        TouchDown,      // 指が触れた（fingerId, x, y に入る）
        TouchMove,      // 触れている指が動いた（fingerId, x, y に入る）
        TouchUp,        // 指が離れた（fingerId, x, y に入る）
    };

    //! 入力の出来事 1 件です。種類ごとに使う項目だけが意味を持ちます。
    struct InputEvent {
        InputEventType type     = InputEventType::KeyDown;    // 出来事の種類
        KeyCode        key      = KeyCode::None;              // 押した・離したキー（KeyDown / KeyUp）
        bool           repeat   = false;                      // キーの押しっぱなしによる繰り返しなら true（KeyDown）
        i32            x        = 0;                          // 位置 X（クライアント領域のピクセル）
        i32            y        = 0;                          // 位置 Y（クライアント領域のピクセル）
        float          wheel    = 0.0f;                       // ホイールの回転量（奥へ回すと正）
        u64            fingerId = 0;                          // 指の識別子（Touch*）
        bool           fromTouch = false;                     // タッチから作られたマウスの出来事なら true
        std::string    text;                                  // 文字列（TextInput / TextEditing）
    };
}    // namespace Tsukino::Input

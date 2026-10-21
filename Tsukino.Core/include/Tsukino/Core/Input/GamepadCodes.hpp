//----------------------------------------------------------------------------
//! @file   GamepadCodes.hpp
//! @brief  ゲームパッドのボタンと入力機器の列挙
//! @detail XInput のボタンに加えて、トリガーと左スティックの傾きを
//!         デジタルのボタンとして読むための値も持ちます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>
// 名前空間 : Tsukino::Input
namespace Tsukino::Input {

    //! ゲームパッドのボタンの列挙型です。
    enum class GamepadButton : u8 {
        // 十字キー
        DPadUp = 0,
        DPadDown,
        DPadLeft,
        DPadRight,

        // 中央のボタン
        Start,
        Back,

        // スティックの押し込み（L3 / R3）
        LeftThumb,
        RightThumb,

        // ショルダー（LB / RB）
        LeftShoulder,
        RightShoulder,

        // フェイスボタン
        A,
        B,
        X,
        Y,

        // トリガー（LT / RT）。しきい値を超えたら押した扱い
        LeftTrigger,
        RightTrigger,

        // 左スティックの傾きを方向キーとして読む分（メニューの移動用）
        LeftStickUp,
        LeftStickDown,
        LeftStickLeft,
        LeftStickRight,

        Count
    };

    //! ボタンの総数です。
    inline constexpr u32 kGamepadButtonCount = static_cast<u32>(GamepadButton::Count);

    //! 同時に扱えるゲームパッドの最大数です（XInput の上限）。
    inline constexpr u32 kMaxGamepads = 4;

    //! 入力機器の種類の列挙型です。
    enum class InputDevice : u8 {
        KeyboardMouse = 0,    // キーボードとマウス
        Gamepad,              // ゲームパッド
    };

    //! スティックの種類の列挙型です。
    enum class GamepadStick : u8 {
        None = 0,    // 割り当て無し
        Left,        // 左スティック
        Right,       // 右スティック
    };

}    // namespace Tsukino::Input

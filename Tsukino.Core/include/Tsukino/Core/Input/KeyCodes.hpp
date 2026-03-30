//--------------------------------------------------------------
//! @file   KeyCodes.hpp
//! @brief  キーコードの定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>
// 名前空間 : Tsukino::Input
namespace Tsukino::Input {
    //--------------------------------------------------------------
    //! @enum   KeyCode
    //! @brief  キーコードの列挙型
    //--------------------------------------------------------------
    enum class KeyCode : u16 {
        None = 0x00,

        // マウスボタン (KeyCodeに含めておくと判定関数を共通化できて便利です)
        LButton = 0x01,
        RButton = 0x02,
        MButton = 0x04,

        // 特殊キー
        Backspace = 0x08,
        Tab       = 0x09,
        Enter     = 0x0D,
        Shift     = 0x10,
        Control   = 0x11,
        Alt       = 0x12,
        Escape    = 0x1B,
        Space     = 0x20,

        // 矢印キー
        Left  = 0x25,
        Up    = 0x26,
        Right = 0x27,
        Down  = 0x28,

        // 数字 (0 - 9)
        D0 = 0x30,
        D1,
        D2,
        D3,
        D4,
        D5,
        D6,
        D7,
        D8,
        D9,

        // アルファベット (A - Z)
        A = 0x41,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        // ファンクションキー
        F1 = 0x70,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
    };

    //--------------------------------------------------------------
    //! @enum   MouseButton
    //! @brief  マウスボタンの列挙型
    //--------------------------------------------------------------
    enum class MouseButton : u8 {
        Left   = 0,
        Right  = 1,
        Middle = 2,
        Max    = 3
    };

}    // namespace Tsukino::Input

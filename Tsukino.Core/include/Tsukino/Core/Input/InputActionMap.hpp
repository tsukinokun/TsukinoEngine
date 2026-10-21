//----------------------------------------------------------------------------
//! @file   InputActionMap.hpp
//! @brief  キー・マウス・ゲームパッドをまとめて「操作」として読むための割り当て
//! @detail 「攻撃」のような操作ごとに、キー・ゲームパッドのボタン・ホイールの割り当てを持ち、
//!         どれか1つでも入力されたら操作されたとみなします。操作の番号はゲーム側の列挙型を
//!         u16 にして使います（エンジンは操作の意味を知りません）。
//----------------------------------------------------------------------------
#pragma once

#include <Tsukino/Core/Input/GamepadCodes.hpp>
#include <Tsukino/Core/Input/KeyCodes.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <hlsl++.h>

#include <string>
#include <vector>
// 名前空間 : Tsukino::Input
namespace Tsukino::Input {
    // 前方宣言
    class InputSystem;

    //! 操作1つぶんの割り当てです。
    struct InputBinding {
        std::vector<KeyCode>       keys;                  //!< キー・マウスボタン（どれか1つで入力）
        std::vector<GamepadButton> buttons;               //!< ゲームパッドのボタン（どれか1つで入力）
        i32                        wheelDirection = 0;    //!< ホイールの向き（+1 で奥へ、−1 で手前へ回したら入力。0 なら使わない）
    };

    //! 2軸の入力（移動など）の割り当てです。
    struct InputAxisBinding {
        InputBinding up;                              //!< +y へ倒すキー
        InputBinding down;                            //!< −y へ倒すキー
        InputBinding left;                            //!< −x へ倒すキー
        InputBinding right;                           //!< +x へ倒すキー
        GamepadStick stick = GamepadStick::None;    //!< 合わせて読むスティック
    };

    //! 割り当てのキーまたはボタンが押されているかを返します（押しっぱなし）。
    //! @param  [in] input   入力システム
    //! @param  [in] binding 割り当て
    //! @return 押されていれば true。ホイールは回したフレームだけ true
    [[nodiscard]]
    bool IsBindingDown(const InputSystem& input, const InputBinding& binding);

    //! 割り当てのキーまたはボタンが押された瞬間かを返します。
    //! @param  [in] input   入力システム
    //! @param  [in] binding 割り当て
    //! @return このフレームに押されたなら true。ホイールは回したフレームだけ true
    [[nodiscard]]
    bool IsBindingPressed(const InputSystem& input, const InputBinding& binding);

    //! 割り当てのキーまたはボタンが離された瞬間かを返します。
    //! @param  [in] input   入力システム
    //! @param  [in] binding 割り当て
    //! @return このフレームに離され、ほかに押しているものが無ければ true
    [[nodiscard]]
    bool IsBindingReleased(const InputSystem& input, const InputBinding& binding);

    //! 2軸の入力を読みます。
    //! @param  [in] input   入力システム
    //! @param  [in] binding 割り当て
    //! @return キーは長さ 1 に正規化した向き、スティックはアナログ値のまま。両方あるときは大きい方
    [[nodiscard]]
    hlslpp::float2 ReadAxis(const InputSystem& input, const InputAxisBinding& binding);

    //! 割り当てを画面に出すときの表記を得ます。
    //! @param  [in] binding 割り当て
    //! @param  [in] device  表記を合わせる入力機器
    //! @return 先頭のキーまたはボタンの表記。その機器の割り当てが無ければ空文字
    [[nodiscard]]
    std::wstring GetBindingDisplayName(const InputBinding& binding, InputDevice device);

    //! 操作ごとの割り当てを持つクラスです。
    class InputActionMap {
    public:
        //! 操作の割り当てを設定します。
        //! @param  [in] actionId 操作の番号（ゲーム側の列挙型の値）
        //! @param  [in] binding  割り当て
        void SetBinding(u16 actionId, InputBinding binding);

        //! 操作の割り当てを得ます。
        //! @param  [in] actionId 操作の番号
        //! @return 割り当て。設定していなければ空の割り当て
        [[nodiscard]]
        const InputBinding& GetBinding(u16 actionId) const;

        //! 操作が入力されているかを返します（押しっぱなし）。
        //! @param  [in] input    入力システム
        //! @param  [in] actionId 操作の番号
        //! @return 入力されていれば true
        [[nodiscard]]
        bool IsDown(const InputSystem& input, u16 actionId) const;

        //! 操作が入力された瞬間かを返します。
        //! @param  [in] input    入力システム
        //! @param  [in] actionId 操作の番号
        //! @return このフレームに入力されたなら true
        [[nodiscard]]
        bool IsPressed(const InputSystem& input, u16 actionId) const;

        //! 操作が離された瞬間かを返します。
        //! @param  [in] input    入力システム
        //! @param  [in] actionId 操作の番号
        //! @return このフレームに離されたなら true
        [[nodiscard]]
        bool IsReleased(const InputSystem& input, u16 actionId) const;

    private:
        std::vector<InputBinding> m_bindings;    //!< 操作の番号ごとの割り当て
    };

}    // namespace Tsukino::Input

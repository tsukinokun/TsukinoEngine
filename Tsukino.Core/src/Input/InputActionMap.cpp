//----------------------------------------------------------------------------
//! @file   InputActionMap.cpp
//! @brief  キー・マウス・ゲームパッドをまとめて「操作」として読むための割り当ての実装
//----------------------------------------------------------------------------
#include <Tsukino/Core/Input/InputActionMap.hpp>
#include <Tsukino/Core/Input/InputNames.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
// 名前空間 : Tsukino::Input
namespace Tsukino::Input {
    // 無名名前空間
    namespace {
        //----------------------------------------------------------------------------
        //! ホイールが割り当ての向きへ回されたかを返します。
        //! @param  [in] input   入力システム
        //! @param  [in] binding 割り当て
        //! @return このフレームに割り当ての向きへ回したなら true
        //----------------------------------------------------------------------------
        bool IsWheelTriggered(const InputSystem& input, const InputBinding& binding) {
            if(binding.wheelDirection == 0)
                return false;
            const float delta = input.GetWheelDelta();
            return binding.wheelDirection > 0 ? delta > 0.0f : delta < 0.0f;
        }

        // 空の割り当て（設定していない操作を引いたときに返す）
        const InputBinding kEmptyBinding{};
    }    // namespace

    //----------------------------------------------------------------------------
    //! 割り当てのキーまたはボタンが押されているかを返します（押しっぱなし）。
    //----------------------------------------------------------------------------
    bool IsBindingDown(const InputSystem& input, const InputBinding& binding) {
        for(KeyCode key : binding.keys) {
            if(input.IsKeyDown(key))
                return true;
        }
        for(GamepadButton button : binding.buttons) {
            if(input.IsButtonDown(button))
                return true;
        }
        return IsWheelTriggered(input, binding);
    }

    //----------------------------------------------------------------------------
    //! 割り当てのキーまたはボタンが押された瞬間かを返します。
    //----------------------------------------------------------------------------
    bool IsBindingPressed(const InputSystem& input, const InputBinding& binding) {
        for(KeyCode key : binding.keys) {
            if(input.IsKeyPressed(key))
                return true;
        }
        for(GamepadButton button : binding.buttons) {
            if(input.IsButtonPressed(button))
                return true;
        }
        return IsWheelTriggered(input, binding);
    }

    //----------------------------------------------------------------------------
    //! 割り当てのキーまたはボタンが離された瞬間かを返します。
    //----------------------------------------------------------------------------
    bool IsBindingReleased(const InputSystem& input, const InputBinding& binding) {
        // 片方を離しても、もう片方を押し続けているなら操作は続いている
        if(IsBindingDown(input, binding))
            return false;

        for(KeyCode key : binding.keys) {
            if(input.IsKeyReleased(key))
                return true;
        }
        for(GamepadButton button : binding.buttons) {
            if(input.IsButtonReleased(button))
                return true;
        }
        return false;
    }

    //----------------------------------------------------------------------------
    //! 2軸の入力を読みます。
    //----------------------------------------------------------------------------
    hlslpp::float2 ReadAxis(const InputSystem& input, const InputAxisBinding& binding) {
        // キーの分。斜めで速くならないよう長さ 1 に揃える
        hlslpp::float2 keyAxis(0.0f, 0.0f);
        if(IsBindingDown(input, binding.up))
            keyAxis.y += 1.0f;
        if(IsBindingDown(input, binding.down))
            keyAxis.y -= 1.0f;
        if(IsBindingDown(input, binding.right))
            keyAxis.x += 1.0f;
        if(IsBindingDown(input, binding.left))
            keyAxis.x -= 1.0f;

        const float keyLength = hlslpp::length(keyAxis);
        if(keyLength > 0.0f)
            keyAxis = keyAxis / keyLength;

        // スティックの分はアナログのまま。キーと同時に入れたときは大きい方を採る
        const hlslpp::float2 stickAxis   = input.GetStick(binding.stick);
        const float          stickLength = hlslpp::length(stickAxis);
        return stickLength > (keyLength > 0.0f ? 1.0f : 0.0f) ? stickAxis : keyAxis;
    }

    //----------------------------------------------------------------------------
    //! 割り当てを画面に出すときの表記を得ます。
    //----------------------------------------------------------------------------
    std::wstring GetBindingDisplayName(const InputBinding& binding, InputDevice device) {
        if(device == InputDevice::Gamepad) {
            if(!binding.buttons.empty())
                return GetGamepadButtonDisplayName(binding.buttons.front());
            return L"";
        }

        if(!binding.keys.empty())
            return GetKeyDisplayName(binding.keys.front());
        if(binding.wheelDirection != 0)
            return L"ホイール";
        return L"";
    }

    //----------------------------------------------------------------------------
    //! 操作の割り当てを設定します。
    //----------------------------------------------------------------------------
    void InputActionMap::SetBinding(u16 actionId, InputBinding binding) {
        if(actionId >= m_bindings.size())
            m_bindings.resize(static_cast<size_t>(actionId) + 1);
        m_bindings[actionId] = std::move(binding);
    }

    //----------------------------------------------------------------------------
    //! 操作の割り当てを得ます。
    //----------------------------------------------------------------------------
    const InputBinding& InputActionMap::GetBinding(u16 actionId) const {
        if(actionId >= m_bindings.size())
            return kEmptyBinding;
        return m_bindings[actionId];
    }

    //----------------------------------------------------------------------------
    //! 操作が入力されているかを返します（押しっぱなし）。
    //----------------------------------------------------------------------------
    bool InputActionMap::IsDown(const InputSystem& input, u16 actionId) const {
        return IsBindingDown(input, GetBinding(actionId));
    }

    //----------------------------------------------------------------------------
    //! 操作が入力された瞬間かを返します。
    //----------------------------------------------------------------------------
    bool InputActionMap::IsPressed(const InputSystem& input, u16 actionId) const {
        return IsBindingPressed(input, GetBinding(actionId));
    }

    //----------------------------------------------------------------------------
    //! 操作が離された瞬間かを返します。
    //----------------------------------------------------------------------------
    bool InputActionMap::IsReleased(const InputSystem& input, u16 actionId) const {
        return IsBindingReleased(input, GetBinding(actionId));
    }

}    // namespace Tsukino::Input

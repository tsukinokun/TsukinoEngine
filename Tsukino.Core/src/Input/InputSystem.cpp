//--------------------------------------------------------------
//! @file   InputSystem.cpp
//! @brief  入力管理システムのロジック実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <algorithm>    // std::copy, std::fill
// 名前空間 : Tsukino::Input
namespace Tsukino::Input {
    //--------------------------------------------------------------
    //! @brief コンストラクタ
    //--------------------------------------------------------------
    InputSystem::InputSystem()
        : m_mouseX(0)
        , m_mouseY(0)
        , m_prevMouseX(0)
        , m_prevMouseY(0)
        , m_wheelDelta(0.0f) {
        // 配列を初期化
        m_currentKeys.fill(false);
        m_previousKeys.fill(false);

        // ゲームパッドへ XInput のスロット番号を割り振る
        for(u32 i = 0; i < kMaxGamepads; ++i)
            m_gamepads[i] = Gamepad(i);
    }

    //--------------------------------------------------------------
    //! @brief 更新関数
    //--------------------------------------------------------------
    void InputSystem::Update() {
        // 現在の状態を「前フレームの状態」として保存する
        // これにより IsKeyPressed (前:false, 今:true) の判定が可能になる
        std::copy(m_currentKeys.begin(), m_currentKeys.end(), m_previousKeys.begin());

        // マウスの座標も更新
        m_prevMouseX = m_mouseX;
        m_prevMouseY = m_mouseY;

        // ホイール量は「そのフレームでの変化量」なので、Updateの最後にリセットする
        m_wheelDelta = 0.0f;
    }

    //--------------------------------------------------------------
    //! @brief ボタンが押されているか（押しっぱなし）
    //--------------------------------------------------------------
    bool InputSystem::IsKeyDown(KeyCode code) const {
        size_t index = static_cast<size_t>(code);
        // 配列外のキーコードは「押されていない」として扱う（範囲外参照を防ぐ）
        if(index >= m_currentKeys.size())
            return false;

        return m_currentKeys[index];
    }

    //--------------------------------------------------------------
    //! @brief ボタンが押された瞬間か
    //--------------------------------------------------------------
    bool InputSystem::IsKeyPressed(KeyCode code) const {
        size_t index = static_cast<size_t>(code);
        if(index >= m_currentKeys.size())
            return false;

        // 前フレームで押されておらず、かつ今フレームで押されている場合のみ true
        return !m_previousKeys[index] && m_currentKeys[index];
    }

    //--------------------------------------------------------------
    //! @brief ボタンが離された瞬間か
    //--------------------------------------------------------------
    bool InputSystem::IsKeyReleased(KeyCode code) const {
        size_t index = static_cast<size_t>(code);
        if(index >= m_currentKeys.size())
            return false;

        // 前フレームで押されており、かつ今フレームで離されている場合のみ true
        return m_previousKeys[index] && !m_currentKeys[index];
    }

    //--------------------------------------------------------------
    //! @brief どのキーでも押された瞬間か
    //--------------------------------------------------------------
    bool InputSystem::AnyKeyPressed() const {
        for(size_t i = 0; i < m_currentKeys.size(); ++i) {
            // 「前フレームで押されておらず、今フレームで押されている」キーが一つでもあれば true
            if(!m_previousKeys[i] && m_currentKeys[i]) {
                return true;
            }
        }
        return false;
    }

    //--------------------------------------------------------------
    //! @brief マウスのスクリーン座標を取得
    //--------------------------------------------------------------
    void InputSystem::GetMousePosition(i32* x, i32* y) const {
        if(x)
            *x = m_mouseX;
        if(y)
            *y = m_mouseY;
    }

    //--------------------------------------------------------------
    //! @brief 前フレームからのマウス移動量を取得
    //--------------------------------------------------------------
    void InputSystem::GetMouseDelta(i32* dx, i32* dy) const {
        if(dx)
            *dx = m_mouseX - m_prevMouseX;
        if(dy)
            *dy = m_mouseY - m_prevMouseY;
    }

    //--------------------------------------------------------------
    //! @brief キー状態を直接書き換える
    //--------------------------------------------------------------
    void InputSystem::SetKeyState(KeyCode code, bool isDown) {
        size_t index = static_cast<size_t>(code);
        if(index < m_currentKeys.size()) {
            m_currentKeys[index] = isDown;
        }
    }

    //--------------------------------------------------------------
    //! @brief マウスの座標を直接書き換える
    //--------------------------------------------------------------
    void InputSystem::SetMousePosition(i32 x, i32 y) {
        m_mouseX = x;
        m_mouseY = y;
    }

    //--------------------------------------------------------------
    //! @brief ホイールの回転量を加算する
    //--------------------------------------------------------------
    void InputSystem::AddWheelDelta(float delta) {
        m_wheelDelta += delta;
    }

    //--------------------------------------------------------------
    //! @brief 全てのキー・ボタンの押下状態をクリアする
    //--------------------------------------------------------------
    void InputSystem::ClearAllKeys() {
        // 前フレーム分もクリアしておく。
        // 現フレームだけクリアすると、フォーカスが戻った直後の1フレームで
        // IsKeyReleased() が「押していないキーを離した」と誤検知する。
        m_currentKeys.fill(false);
        m_previousKeys.fill(false);

        m_wheelDelta = 0.0f;

        // 震えたまま別のウィンドウへ移ると止める機会が無くなるので、ここで止める
        StopGamepadVibration();
    }

    //--------------------------------------------------------------
    //! キーボードかマウスが操作されたことを知らせます。
    //--------------------------------------------------------------
    void InputSystem::NotifyKeyboardMouseActivity() {
        m_lastInputDevice = InputDevice::KeyboardMouse;
    }

    //--------------------------------------------------------------
    //! 全スロットのゲームパッドの状態を読み直します。
    //--------------------------------------------------------------
    void InputSystem::PollGamepads(float deltaTime, bool windowFocused) {
        // スティックをこれ以上倒したら「ゲームパッドを操作した」とみなす。
        // デッドゾーンより大きくしておき、置いてあるだけのパッドの揺れで表示が切り替わらないようにする
        constexpr float kActivityStickThreshold   = 0.5f;
        constexpr float kActivityTriggerThreshold = 0.5f;

        for(Gamepad& gamepad : m_gamepads) {
            gamepad.Poll(deltaTime, windowFocused);
            if(!gamepad.IsConnected())
                continue;

            const float leftLength  = hlslpp::length(gamepad.GetLeftStick());
            const float rightLength = hlslpp::length(gamepad.GetRightStick());
            const bool  stickMoved  = leftLength >= kActivityStickThreshold || rightLength >= kActivityStickThreshold;
            const bool triggerPulled =
                gamepad.GetLeftTrigger() >= kActivityTriggerThreshold || gamepad.GetRightTrigger() >= kActivityTriggerThreshold;

            if(gamepad.AnyButtonPressed() || stickMoved || triggerPulled)
                m_lastInputDevice = InputDevice::Gamepad;
        }
    }

    //--------------------------------------------------------------
    //! スロットを指定してゲームパッドを得ます。
    //--------------------------------------------------------------
    const Gamepad& InputSystem::GetGamepad(u32 index) const {
        // 範囲外は先頭のスロットを返す（未接続なら何も押していない状態が読める）
        if(index >= kMaxGamepads)
            index = 0;
        return m_gamepads[index];
    }

    //--------------------------------------------------------------
    //! いずれかのゲームパッドでボタンが押されているかを返します。
    //--------------------------------------------------------------
    bool InputSystem::IsButtonDown(GamepadButton button) const {
        for(const Gamepad& gamepad : m_gamepads) {
            if(gamepad.IsButtonDown(button))
                return true;
        }
        return false;
    }

    //--------------------------------------------------------------
    //! いずれかのゲームパッドでボタンが押された瞬間かを返します。
    //--------------------------------------------------------------
    bool InputSystem::IsButtonPressed(GamepadButton button) const {
        for(const Gamepad& gamepad : m_gamepads) {
            if(gamepad.IsButtonPressed(button))
                return true;
        }
        return false;
    }

    //--------------------------------------------------------------
    //! いずれかのゲームパッドでボタンが離された瞬間かを返します。
    //--------------------------------------------------------------
    bool InputSystem::IsButtonReleased(GamepadButton button) const {
        for(const Gamepad& gamepad : m_gamepads) {
            if(gamepad.IsButtonReleased(button))
                return true;
        }
        return false;
    }

    //--------------------------------------------------------------
    //! 左スティックの傾きを返します。
    //--------------------------------------------------------------
    hlslpp::float2 InputSystem::GetLeftStick() const {
        return GetStick(GamepadStick::Left);
    }

    //--------------------------------------------------------------
    //! 右スティックの傾きを返します。
    //--------------------------------------------------------------
    hlslpp::float2 InputSystem::GetRightStick() const {
        return GetStick(GamepadStick::Right);
    }

    //--------------------------------------------------------------
    //! 指定したスティックの傾きを返します。
    //--------------------------------------------------------------
    hlslpp::float2 InputSystem::GetStick(GamepadStick stick) const {
        hlslpp::float2 result(0.0f, 0.0f);
        if(stick == GamepadStick::None)
            return result;

        // 複数台の値を足すと 1 を超えるので、最も大きく倒している1台の値を採る
        float bestLength = 0.0f;
        for(const Gamepad& gamepad : m_gamepads) {
            const hlslpp::float2 value  = (stick == GamepadStick::Left) ? gamepad.GetLeftStick() : gamepad.GetRightStick();
            const float          length = hlslpp::length(value);
            if(length > bestLength) {
                bestLength = length;
                result     = value;
            }
        }
        return result;
    }

    //--------------------------------------------------------------
    //! 接続されている全てのゲームパッドを振動させます。
    //--------------------------------------------------------------
    void InputSystem::SetGamepadVibration(float lowFrequency, float highFrequency, float durationSeconds) {
        for(Gamepad& gamepad : m_gamepads)
            gamepad.SetVibration(lowFrequency, highFrequency, durationSeconds);
    }

    //--------------------------------------------------------------
    //! 全てのゲームパッドの振動を止めます。
    //--------------------------------------------------------------
    void InputSystem::StopGamepadVibration() {
        for(Gamepad& gamepad : m_gamepads)
            gamepad.StopVibration();
    }

    //--------------------------------------------------------------
    //! 全てのゲームパッドのスティックのデッドゾーンを設定します。
    //--------------------------------------------------------------
    void InputSystem::SetGamepadStickDeadZone(float deadZone) {
        for(Gamepad& gamepad : m_gamepads)
            gamepad.SetStickDeadZone(deadZone);
    }

    //--------------------------------------------------------------
    //! 全てのゲームパッドのトリガーのしきい値を設定します。
    //--------------------------------------------------------------
    void InputSystem::SetGamepadTriggerThreshold(float threshold) {
        for(Gamepad& gamepad : m_gamepads)
            gamepad.SetTriggerThreshold(threshold);
    }

    //--------------------------------------------------------------
    //! いずれかのゲームパッドが接続されているかを返します。
    //--------------------------------------------------------------
    bool InputSystem::IsAnyGamepadConnected() const {
        for(const Gamepad& gamepad : m_gamepads) {
            if(gamepad.IsConnected())
                return true;
        }
        return false;
    }

}    // namespace Tsukino::Input

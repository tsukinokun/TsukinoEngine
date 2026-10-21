//----------------------------------------------------------------------------
//! @file   Gamepad.cpp
//! @brief  ゲームパッド1台ぶんの状態の読み取りと振動の実装
//----------------------------------------------------------------------------
#include <Tsukino/Core/Input/Gamepad.hpp>
#include <Tsukino/Core/Log.hpp>

#include <Xinput.h>
#pragma comment(lib, "xinput.lib")

#include <algorithm>
#include <cmath>
#include <string>
// 名前空間 : Tsukino::Input
namespace Tsukino::Input {
    // 無名名前空間
    namespace {
        // 未接続のスロットを確かめ直す間隔（秒）。
        // 繋がっていないスロットへの XInputGetState は数ミリ秒かかることがあるため、毎フレームは読まない
        constexpr float kReconnectIntervalSeconds = 1.0f;

        //----------------------------------------------------------------------------
        //! スティックの生の値へ円形のデッドゾーンをかけ、−1〜1 へ正規化します。
        //! @param  [in] rawX     X の生の値（−32768〜32767）
        //! @param  [in] rawY     Y の生の値（−32768〜32767）
        //! @param  [in] deadZone デッドゾーン（0〜1）
        //! @return デッドゾーンの外側を 0〜1 へ引き伸ばした値
        //----------------------------------------------------------------------------
        hlslpp::float2 ApplyRadialDeadZone(SHORT rawX, SHORT rawY, float deadZone) {
            // −32768 側が 1 段ぶん長いので、32767 で割って ±1 へ丸める
            float x = std::clamp(static_cast<float>(rawX) / 32767.0f, -1.0f, 1.0f);
            float y = std::clamp(static_cast<float>(rawY) / 32767.0f, -1.0f, 1.0f);

            const float length = std::sqrt(x * x + y * y);
            if(length <= deadZone)
                return hlslpp::float2(0.0f, 0.0f);

            // デッドゾーンの縁を 0 として、縁から外周までを 0〜1 に引き伸ばす
            // （そのままだと、倒し始めた瞬間にいきなり deadZone ぶんの速さが出る）
            const float clampedLength = std::min(length, 1.0f);
            const float scaled        = (clampedLength - deadZone) / (1.0f - deadZone);
            return hlslpp::float2(x / length * scaled, y / length * scaled);
        }

        //----------------------------------------------------------------------------
        //! 左スティックの方向ボタンの押下状態を、ヒステリシスつきで決めます。
        //! @param  [in] value      その方向への傾き（0〜1。逆向きは負）
        //! @param  [in] wasDown    前フレームに押した扱いだったか
        //! @return このフレームに押した扱いにするなら true
        //----------------------------------------------------------------------------
        bool ResolveStickButton(float value, bool wasDown) {
            // 押したと離したのしきい値を分け、境目でガタガタと押し直しが起きないようにする
            const float threshold = wasDown ? Gamepad::kStickButtonReleaseThreshold : Gamepad::kStickButtonPressThreshold;
            return value >= threshold;
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 状態を読むスロットを指定して初期化します。
    //----------------------------------------------------------------------------
    Gamepad::Gamepad(u32 userIndex)
        : m_userIndex(userIndex) {
        ResetState();
    }

    //----------------------------------------------------------------------------
    //! 状態を読み直し、振動の残り時間を進めます。
    //----------------------------------------------------------------------------
    void Gamepad::Poll(float deltaTime, bool active) {
        // 前フレームの状態として保存する（IsButtonPressed の判定に使う）
        m_previousButtons = m_currentButtons;

        // 読まないフレームは、押しっぱなしが残らないよう中立にしておく
        if(!active) {
            m_currentButtons.fill(false);
            m_leftStick    = hlslpp::float2(0.0f, 0.0f);
            m_rightStick   = hlslpp::float2(0.0f, 0.0f);
            m_leftTrigger  = 0.0f;
            m_rightTrigger = 0.0f;
            StopVibration();
            return;
        }

        // 未接続のスロットは間隔を空けて確かめる
        if(!m_connected) {
            m_reconnectTimer -= deltaTime;
            if(m_reconnectTimer > 0.0f)
                return;
            m_reconnectTimer = kReconnectIntervalSeconds;
        }

        XINPUT_STATE state{};
        if(XInputGetState(m_userIndex, &state) != ERROR_SUCCESS) {
            if(m_connected)
                Tsukino::Core::Log::Info("Gamepad " + std::to_string(m_userIndex) + " disconnected.");

            m_connected      = false;
            m_reconnectTimer = kReconnectIntervalSeconds;
            m_vibrating      = false;
            m_vibrationRemaining = 0.0f;
            // 抜けた瞬間に押していたボタンが「離した」と判定されるよう、前フレームは残して今だけ空にする
            m_currentButtons.fill(false);
            m_leftStick    = hlslpp::float2(0.0f, 0.0f);
            m_rightStick   = hlslpp::float2(0.0f, 0.0f);
            m_leftTrigger  = 0.0f;
            m_rightTrigger = 0.0f;
            return;
        }

        const bool justConnected = !m_connected;
        if(justConnected) {
            Tsukino::Core::Log::Info("Gamepad " + std::to_string(m_userIndex) + " connected.");
            m_connected = true;
        }

        const XINPUT_GAMEPAD& pad = state.Gamepad;

        //----------------------------------------------------------------------------
        // アナログ値
        //----------------------------------------------------------------------------
        m_leftStick  = ApplyRadialDeadZone(pad.sThumbLX, pad.sThumbLY, m_stickDeadZone);
        m_rightStick = ApplyRadialDeadZone(pad.sThumbRX, pad.sThumbRY, m_stickDeadZone);

        m_leftTrigger  = static_cast<float>(pad.bLeftTrigger) / 255.0f;
        m_rightTrigger = static_cast<float>(pad.bRightTrigger) / 255.0f;

        //----------------------------------------------------------------------------
        // 物理ボタン
        //----------------------------------------------------------------------------
        auto setButton = [this](GamepadButton button, bool isDown) { m_currentButtons[static_cast<size_t>(button)] = isDown; };
        auto wasDown   = [this](GamepadButton button) { return m_previousButtons[static_cast<size_t>(button)]; };

        const WORD buttons = pad.wButtons;
        setButton(GamepadButton::DPadUp, (buttons & XINPUT_GAMEPAD_DPAD_UP) != 0);
        setButton(GamepadButton::DPadDown, (buttons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
        setButton(GamepadButton::DPadLeft, (buttons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
        setButton(GamepadButton::DPadRight, (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
        setButton(GamepadButton::Start, (buttons & XINPUT_GAMEPAD_START) != 0);
        setButton(GamepadButton::Back, (buttons & XINPUT_GAMEPAD_BACK) != 0);
        setButton(GamepadButton::LeftThumb, (buttons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
        setButton(GamepadButton::RightThumb, (buttons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
        setButton(GamepadButton::LeftShoulder, (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
        setButton(GamepadButton::RightShoulder, (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
        setButton(GamepadButton::A, (buttons & XINPUT_GAMEPAD_A) != 0);
        setButton(GamepadButton::B, (buttons & XINPUT_GAMEPAD_B) != 0);
        setButton(GamepadButton::X, (buttons & XINPUT_GAMEPAD_X) != 0);
        setButton(GamepadButton::Y, (buttons & XINPUT_GAMEPAD_Y) != 0);

        //----------------------------------------------------------------------------
        // アナログ値をボタンとして読む分
        //----------------------------------------------------------------------------
        setButton(GamepadButton::LeftTrigger, m_leftTrigger >= m_triggerThreshold);
        setButton(GamepadButton::RightTrigger, m_rightTrigger >= m_triggerThreshold);

        setButton(GamepadButton::LeftStickUp, ResolveStickButton(m_leftStick.y, wasDown(GamepadButton::LeftStickUp)));
        setButton(GamepadButton::LeftStickDown, ResolveStickButton(-m_leftStick.y, wasDown(GamepadButton::LeftStickDown)));
        setButton(GamepadButton::LeftStickLeft, ResolveStickButton(-m_leftStick.x, wasDown(GamepadButton::LeftStickLeft)));
        setButton(GamepadButton::LeftStickRight, ResolveStickButton(m_leftStick.x, wasDown(GamepadButton::LeftStickRight)));

        // 繋いだ瞬間に押していたボタンを「押した瞬間」と取り違えないよう、前フレームも今の状態に揃える
        if(justConnected)
            m_previousButtons = m_currentButtons;

        //----------------------------------------------------------------------------
        // 振動の残り時間
        //----------------------------------------------------------------------------
        if(m_vibrating) {
            m_vibrationRemaining -= deltaTime;
            if(m_vibrationRemaining <= 0.0f)
                StopVibration();
        }
    }

    //----------------------------------------------------------------------------
    //! ボタンが押されているかを返します（押しっぱなし）。
    //----------------------------------------------------------------------------
    bool Gamepad::IsButtonDown(GamepadButton button) const {
        const size_t index = static_cast<size_t>(button);
        if(index >= m_currentButtons.size())
            return false;
        return m_currentButtons[index];
    }

    //----------------------------------------------------------------------------
    //! ボタンが押された瞬間かを返します。
    //----------------------------------------------------------------------------
    bool Gamepad::IsButtonPressed(GamepadButton button) const {
        const size_t index = static_cast<size_t>(button);
        if(index >= m_currentButtons.size())
            return false;
        return !m_previousButtons[index] && m_currentButtons[index];
    }

    //----------------------------------------------------------------------------
    //! ボタンが離された瞬間かを返します。
    //----------------------------------------------------------------------------
    bool Gamepad::IsButtonReleased(GamepadButton button) const {
        const size_t index = static_cast<size_t>(button);
        if(index >= m_currentButtons.size())
            return false;
        return m_previousButtons[index] && !m_currentButtons[index];
    }

    //----------------------------------------------------------------------------
    //! どれかのボタンが押された瞬間かを返します。
    //----------------------------------------------------------------------------
    bool Gamepad::AnyButtonPressed() const {
        for(size_t i = 0; i < m_currentButtons.size(); ++i) {
            if(!m_previousButtons[i] && m_currentButtons[i])
                return true;
        }
        return false;
    }

    //----------------------------------------------------------------------------
    //! 振動させます。すでに振動中なら強さと長さを上書きします。
    //----------------------------------------------------------------------------
    void Gamepad::SetVibration(float lowFrequency, float highFrequency, float durationSeconds) {
        if(!m_connected || durationSeconds <= 0.0f)
            return;

        m_vibrationRemaining = durationSeconds;
        ApplyVibration(lowFrequency, highFrequency);
    }

    //----------------------------------------------------------------------------
    //! 振動を止めます。
    //----------------------------------------------------------------------------
    void Gamepad::StopVibration() {
        m_vibrationRemaining = 0.0f;
        if(!m_vibrating)
            return;
        ApplyVibration(0.0f, 0.0f);
    }

    //----------------------------------------------------------------------------
    //! スティックのデッドゾーンを設定します。
    //----------------------------------------------------------------------------
    void Gamepad::SetStickDeadZone(float deadZone) {
        // 1 にすると正規化で 0 除算になるので、上限を少し手前で止める
        m_stickDeadZone = std::clamp(deadZone, 0.0f, 0.95f);
    }

    //----------------------------------------------------------------------------
    //! トリガーを押した扱いにするしきい値を設定します。
    //----------------------------------------------------------------------------
    void Gamepad::SetTriggerThreshold(float threshold) {
        m_triggerThreshold = std::clamp(threshold, 0.0f, 1.0f);
    }

    //----------------------------------------------------------------------------
    //! 状態を中立（何も押していない）にします。
    //----------------------------------------------------------------------------
    void Gamepad::ResetState() {
        m_currentButtons.fill(false);
        m_previousButtons.fill(false);
        m_leftStick    = hlslpp::float2(0.0f, 0.0f);
        m_rightStick   = hlslpp::float2(0.0f, 0.0f);
        m_leftTrigger  = 0.0f;
        m_rightTrigger = 0.0f;
    }

    //----------------------------------------------------------------------------
    //! 振動の強さをモーターへ書き込みます。
    //----------------------------------------------------------------------------
    void Gamepad::ApplyVibration(float lowFrequency, float highFrequency) {
        XINPUT_VIBRATION vibration{};
        vibration.wLeftMotorSpeed  = static_cast<WORD>(std::clamp(lowFrequency, 0.0f, 1.0f) * 65535.0f);
        vibration.wRightMotorSpeed = static_cast<WORD>(std::clamp(highFrequency, 0.0f, 1.0f) * 65535.0f);
        XInputSetState(m_userIndex, &vibration);

        m_vibrating = vibration.wLeftMotorSpeed != 0 || vibration.wRightMotorSpeed != 0;
    }

}    // namespace Tsukino::Input

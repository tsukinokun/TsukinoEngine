//----------------------------------------------------------------------------
//! @file   Gamepad.hpp
//! @brief  ゲームパッド1台ぶんの状態の読み取りと振動
//! @detail XInput で毎フレーム状態を読み、ボタンの押下・スティック・トリガーの
//!         値を判定しやすい形で持ちます。振動は長さを指定して出し、時間が来たら自動で止めます。
//----------------------------------------------------------------------------
#pragma once

#include <Tsukino/Core/Input/GamepadCodes.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <hlsl++.h>

#include <array>
// 名前空間 : Tsukino::Input
namespace Tsukino::Input {

    //! ゲームパッド1台ぶんの状態を持つクラスです。
    class Gamepad {
    public:
        //! デッドゾーン（0〜1）の既定値です。これ未満の傾きは 0 として扱います。
        static constexpr float kDefaultStickDeadZone = 0.24f;

        //! トリガーを押した扱いにするしきい値（0〜1）の既定値です。
        static constexpr float kDefaultTriggerThreshold = 0.3f;

        //! 左スティックの傾きを方向キーとして押した扱いにするしきい値です。
        static constexpr float kStickButtonPressThreshold = 0.6f;

        //! 左スティックの傾きを方向キーとして離した扱いにするしきい値です（ヒステリシス）。
        static constexpr float kStickButtonReleaseThreshold = 0.4f;

        //! 状態を読むスロットを指定して初期化します。
        //! @param  [in] userIndex XInput のスロット番号（0〜3）。
        explicit Gamepad(u32 userIndex = 0);

        //! 状態を読み直し、振動の残り時間を進めます。
        //! @param  [in] deltaTime 前フレームからの経過秒数。
        //! @param  [in] active    false のときは状態を読まず、中立の状態にして振動も止めます。
        void Poll(float deltaTime, bool active);

        //! 接続されているかを返します。
        //! @return 接続されていれば true。
        [[nodiscard]]
        bool IsConnected() const {
            return m_connected;
        }

        //! ボタンが押されているかを返します（押しっぱなし）。
        //! @param  [in] button 判定するボタン。
        //! @return 押されていれば true。
        [[nodiscard]]
        bool IsButtonDown(GamepadButton button) const;

        //! ボタンが押された瞬間かを返します。
        //! @param  [in] button 判定するボタン。
        //! @return このフレームに押されたなら true。
        [[nodiscard]]
        bool IsButtonPressed(GamepadButton button) const;

        //! ボタンが離された瞬間かを返します。
        //! @param  [in] button 判定するボタン。
        //! @return このフレームに離されたなら true。
        [[nodiscard]]
        bool IsButtonReleased(GamepadButton button) const;

        //! どれかのボタンが押された瞬間かを返します。
        //! @return このフレームに押されたボタンが1つでもあれば true。
        [[nodiscard]]
        bool AnyButtonPressed() const;

        //! 左スティックの傾きを返します。
        //! @return デッドゾーンを除いて −1〜1 に正規化した値（上が +y）。
        [[nodiscard]]
        hlslpp::float2 GetLeftStick() const {
            return m_leftStick;
        }

        //! 右スティックの傾きを返します。
        //! @return デッドゾーンを除いて −1〜1 に正規化した値（上が +y）。
        [[nodiscard]]
        hlslpp::float2 GetRightStick() const {
            return m_rightStick;
        }

        //! 左トリガーの押し込みを返します。
        //! @return 0〜1 の値。
        [[nodiscard]]
        float GetLeftTrigger() const {
            return m_leftTrigger;
        }

        //! 右トリガーの押し込みを返します。
        //! @return 0〜1 の値。
        [[nodiscard]]
        float GetRightTrigger() const {
            return m_rightTrigger;
        }

        //! 振動させます。すでに振動中なら強さと長さを上書きします。
        //! @param  [in] lowFrequency    左側（低周波・重い）モーターの強さ（0〜1）。
        //! @param  [in] highFrequency   右側（高周波・軽い）モーターの強さ（0〜1）。
        //! @param  [in] durationSeconds 振動させる秒数。過ぎると自動で止まります。
        void SetVibration(float lowFrequency, float highFrequency, float durationSeconds);

        //! 振動を止めます。
        void StopVibration();

        //! スティックのデッドゾーンを設定します。
        //! @param  [in] deadZone 0〜1 の値。
        void SetStickDeadZone(float deadZone);

        //! トリガーを押した扱いにするしきい値を設定します。
        //! @param  [in] threshold 0〜1 の値。
        void SetTriggerThreshold(float threshold);

    private:
        //! 状態を中立（何も押していない）にします。
        void ResetState();

        //! 振動の強さをモーターへ書き込みます。
        //! @param  [in] lowFrequency  左側モーターの強さ（0〜1）。
        //! @param  [in] highFrequency 右側モーターの強さ（0〜1）。
        void ApplyVibration(float lowFrequency, float highFrequency);

        u32 m_userIndex = 0;    //!< XInput のスロット番号

        bool  m_connected      = false;    //!< 接続されているか
        float m_reconnectTimer = 0.0f;     //!< 未接続のとき、次に接続を確かめるまでの秒数

        std::array<bool, kGamepadButtonCount> m_currentButtons{};     //!< 現フレームのボタン状態
        std::array<bool, kGamepadButtonCount> m_previousButtons{};    //!< 前フレームのボタン状態

        hlslpp::float2 m_leftStick  = hlslpp::float2(0.0f, 0.0f);    //!< 左スティック
        hlslpp::float2 m_rightStick = hlslpp::float2(0.0f, 0.0f);    //!< 右スティック
        float          m_leftTrigger  = 0.0f;                         //!< 左トリガー
        float          m_rightTrigger = 0.0f;                         //!< 右トリガー

        float m_stickDeadZone    = kDefaultStickDeadZone;       //!< スティックのデッドゾーン
        float m_triggerThreshold = kDefaultTriggerThreshold;    //!< トリガーのしきい値

        float m_vibrationRemaining = 0.0f;     //!< 振動の残り秒数（0 なら止まっている）
        bool  m_vibrating          = false;    //!< モーターへ 0 以外を書き込んでいるか
    };

}    // namespace Tsukino::Input

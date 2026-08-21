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
    }

}    // namespace Tsukino::Input

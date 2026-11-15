//----------------------------------------------------------------------------
//! @file   InputNames.cpp
//! @brief  キー・ゲームパッドのボタンと名前の相互変換の実装
//----------------------------------------------------------------------------
#include <Tsukino/Core/Input/InputNames.hpp>

#include <array>
#include <cctype>
// 名前空間 : Tsukino::Input
namespace Tsukino::Input {
    // 無名名前空間
    namespace {
        //! キー1つぶんの綴りと表記です。
        struct KeyNameEntry {
            KeyCode        code;           // キーコード
            const char*    name;           // 設定ファイルの綴り
            const char*    displayName;    // 画面の表記
        };

        //! ボタン1つぶんの綴りと表記です。
        struct ButtonNameEntry {
            GamepadButton  button;         // ボタン
            const char*    name;           // 設定ファイルの綴り
            const char*    displayName;    // 画面の表記
        };

        // 英字・数字・ファンクションキーは規則的なので、表に載せず計算で引く
        constexpr std::array kKeyNames = {
            KeyNameEntry{ KeyCode::LButton, "LButton", "左クリック" },
            KeyNameEntry{ KeyCode::RButton, "RButton", "右クリック" },
            KeyNameEntry{ KeyCode::MButton, "MButton", "中クリック" },
            KeyNameEntry{ KeyCode::Backspace, "Backspace", "BS" },
            KeyNameEntry{ KeyCode::Tab, "Tab", "TAB" },
            KeyNameEntry{ KeyCode::Enter, "Enter", "ENTER" },
            KeyNameEntry{ KeyCode::Shift, "Shift", "SHIFT" },
            KeyNameEntry{ KeyCode::Control, "Control", "CTR" },
            KeyNameEntry{ KeyCode::Alt, "Alt", "ALT" },
            KeyNameEntry{ KeyCode::Escape, "Escape", "Esc" },
            KeyNameEntry{ KeyCode::Space, "Space", "SPACE" },
            KeyNameEntry{ KeyCode::PageUp, "PageUp", "PgUp" },
            KeyNameEntry{ KeyCode::PageDown, "PageDown", "PgDn" },
            KeyNameEntry{ KeyCode::End, "End", "END" },
            KeyNameEntry{ KeyCode::Home, "Home", "HOME" },
            KeyNameEntry{ KeyCode::Left, "Left", "←" },
            KeyNameEntry{ KeyCode::Up, "Up", "↑" },
            KeyNameEntry{ KeyCode::Right, "Right", "→" },
            KeyNameEntry{ KeyCode::Down, "Down", "↓" },
        };

        constexpr std::array kButtonNames = {
            ButtonNameEntry{ GamepadButton::DPadUp, "DPadUp", "↑" },
            ButtonNameEntry{ GamepadButton::DPadDown, "DPadDown", "↓" },
            ButtonNameEntry{ GamepadButton::DPadLeft, "DPadLeft", "←" },
            ButtonNameEntry{ GamepadButton::DPadRight, "DPadRight", "→" },
            ButtonNameEntry{ GamepadButton::Start, "Start", "START" },
            ButtonNameEntry{ GamepadButton::Back, "Back", "BACK" },
            ButtonNameEntry{ GamepadButton::LeftThumb, "LeftThumb", "L3" },
            ButtonNameEntry{ GamepadButton::RightThumb, "RightThumb", "R3" },
            ButtonNameEntry{ GamepadButton::LeftShoulder, "LeftShoulder", "LB" },
            ButtonNameEntry{ GamepadButton::RightShoulder, "RightShoulder", "RB" },
            ButtonNameEntry{ GamepadButton::A, "A", "A" },
            ButtonNameEntry{ GamepadButton::B, "B", "B" },
            ButtonNameEntry{ GamepadButton::X, "X", "X" },
            ButtonNameEntry{ GamepadButton::Y, "Y", "Y" },
            ButtonNameEntry{ GamepadButton::LeftTrigger, "LeftTrigger", "LT" },
            ButtonNameEntry{ GamepadButton::RightTrigger, "RightTrigger", "RT" },
            ButtonNameEntry{ GamepadButton::LeftStickUp, "LeftStickUp", "L↑" },
            ButtonNameEntry{ GamepadButton::LeftStickDown, "LeftStickDown", "L↓" },
            ButtonNameEntry{ GamepadButton::LeftStickLeft, "LeftStickLeft", "L←" },
            ButtonNameEntry{ GamepadButton::LeftStickRight, "LeftStickRight", "L→" },
        };

        // 1文字のキー表記（英字・数字）の置き場。表記は呼び出し側へポインタで返すので、静的に持っておく
        constexpr std::array<char[2], 36> kSingleCharNames = { {
            { 'A', 0 }, { 'B', 0 }, { 'C', 0 }, { 'D', 0 }, { 'E', 0 }, { 'F', 0 }, { 'G', 0 }, { 'H', 0 }, { 'I', 0 },
            { 'J', 0 }, { 'K', 0 }, { 'L', 0 }, { 'M', 0 }, { 'N', 0 }, { 'O', 0 }, { 'P', 0 }, { 'Q', 0 }, { 'R', 0 },
            { 'S', 0 }, { 'T', 0 }, { 'U', 0 }, { 'V', 0 }, { 'W', 0 }, { 'X', 0 }, { 'Y', 0 }, { 'Z', 0 }, { '0', 0 },
            { '1', 0 }, { '2', 0 }, { '3', 0 }, { '4', 0 }, { '5', 0 }, { '6', 0 }, { '7', 0 }, { '8', 0 }, { '9', 0 },
        } };

        constexpr std::array kFunctionKeyNames = { "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12" };

        //----------------------------------------------------------------------------
        //! 大文字小文字を区別せずに文字列を比べます。
        //! @param  [in] a 比べる文字列
        //! @param  [in] b 比べる文字列
        //! @return 同じ綴りなら true
        //----------------------------------------------------------------------------
        bool EqualsIgnoreCase(std::string_view a, std::string_view b) {
            if(a.size() != b.size())
                return false;
            for(size_t i = 0; i < a.size(); ++i) {
                if(std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
                    return false;
            }
            return true;
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 設定ファイルの綴りからキーコードを得ます。
    //----------------------------------------------------------------------------
    KeyCode KeyCodeFromName(std::string_view name) {
        for(const KeyNameEntry& entry : kKeyNames) {
            if(EqualsIgnoreCase(name, entry.name))
                return entry.code;
        }

        // 英字1文字（"W"）
        if(name.size() == 1 && std::isalpha(static_cast<unsigned char>(name[0])))
            return static_cast<KeyCode>(static_cast<u16>(KeyCode::A) + (std::toupper(static_cast<unsigned char>(name[0])) - 'A'));

        // 数字（"D1"）
        if(name.size() == 2 && (name[0] == 'D' || name[0] == 'd') && std::isdigit(static_cast<unsigned char>(name[1])))
            return static_cast<KeyCode>(static_cast<u16>(KeyCode::D0) + (name[1] - '0'));

        // ファンクションキー（"F1"〜"F12"）
        if(name.size() >= 2 && (name[0] == 'F' || name[0] == 'f')) {
            int number = 0;
            for(size_t i = 1; i < name.size(); ++i) {
                if(!std::isdigit(static_cast<unsigned char>(name[i])))
                    return KeyCode::None;
                number = number * 10 + (name[i] - '0');
            }
            if(number >= 1 && number <= 12)
                return static_cast<KeyCode>(static_cast<u16>(KeyCode::F1) + (number - 1));
        }

        return KeyCode::None;
    }

    //----------------------------------------------------------------------------
    //! 設定ファイルの綴りからゲームパッドのボタンを得ます。
    //----------------------------------------------------------------------------
    GamepadButton GamepadButtonFromName(std::string_view name) {
        for(const ButtonNameEntry& entry : kButtonNames) {
            if(EqualsIgnoreCase(name, entry.name))
                return entry.button;
        }
        return GamepadButton::Count;
    }

    //----------------------------------------------------------------------------
    //! キーを画面に出すときの表記を得ます。
    //----------------------------------------------------------------------------
    const char* GetKeyDisplayName(KeyCode code) {
        for(const KeyNameEntry& entry : kKeyNames) {
            if(entry.code == code)
                return entry.displayName;
        }

        const u16 value = static_cast<u16>(code);
        if(value >= static_cast<u16>(KeyCode::A) && value <= static_cast<u16>(KeyCode::Z))
            return kSingleCharNames[value - static_cast<u16>(KeyCode::A)];
        if(value >= static_cast<u16>(KeyCode::D0) && value <= static_cast<u16>(KeyCode::D9))
            return kSingleCharNames[26 + (value - static_cast<u16>(KeyCode::D0))];
        if(value >= static_cast<u16>(KeyCode::F1) && value <= static_cast<u16>(KeyCode::F12))
            return kFunctionKeyNames[value - static_cast<u16>(KeyCode::F1)];

        return "";
    }

    //----------------------------------------------------------------------------
    //! ゲームパッドのボタンを画面に出すときの表記を得ます。
    //----------------------------------------------------------------------------
    const char* GetGamepadButtonDisplayName(GamepadButton button) {
        for(const ButtonNameEntry& entry : kButtonNames) {
            if(entry.button == button)
                return entry.displayName;
        }
        return "";
    }

}    // namespace Tsukino::Input

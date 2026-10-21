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
            const wchar_t* displayName;    // 画面の表記
        };

        //! ボタン1つぶんの綴りと表記です。
        struct ButtonNameEntry {
            GamepadButton  button;         // ボタン
            const char*    name;           // 設定ファイルの綴り
            const wchar_t* displayName;    // 画面の表記
        };

        // 英字・数字・ファンクションキーは規則的なので、表に載せず計算で引く
        constexpr std::array kKeyNames = {
            KeyNameEntry{ KeyCode::LButton, "LButton", L"左クリック" },
            KeyNameEntry{ KeyCode::RButton, "RButton", L"右クリック" },
            KeyNameEntry{ KeyCode::MButton, "MButton", L"中クリック" },
            KeyNameEntry{ KeyCode::Backspace, "Backspace", L"BS" },
            KeyNameEntry{ KeyCode::Tab, "Tab", L"TAB" },
            KeyNameEntry{ KeyCode::Enter, "Enter", L"ENTER" },
            KeyNameEntry{ KeyCode::Shift, "Shift", L"SHIFT" },
            KeyNameEntry{ KeyCode::Control, "Control", L"CTRL" },
            KeyNameEntry{ KeyCode::Alt, "Alt", L"ALT" },
            KeyNameEntry{ KeyCode::Escape, "Escape", L"Esc" },
            KeyNameEntry{ KeyCode::Space, "Space", L"SPACE" },
            KeyNameEntry{ KeyCode::PageUp, "PageUp", L"PgUp" },
            KeyNameEntry{ KeyCode::PageDown, "PageDown", L"PgDn" },
            KeyNameEntry{ KeyCode::End, "End", L"END" },
            KeyNameEntry{ KeyCode::Home, "Home", L"HOME" },
            KeyNameEntry{ KeyCode::Left, "Left", L"←" },
            KeyNameEntry{ KeyCode::Up, "Up", L"↑" },
            KeyNameEntry{ KeyCode::Right, "Right", L"→" },
            KeyNameEntry{ KeyCode::Down, "Down", L"↓" },
        };

        constexpr std::array kButtonNames = {
            ButtonNameEntry{ GamepadButton::DPadUp, "DPadUp", L"↑" },
            ButtonNameEntry{ GamepadButton::DPadDown, "DPadDown", L"↓" },
            ButtonNameEntry{ GamepadButton::DPadLeft, "DPadLeft", L"←" },
            ButtonNameEntry{ GamepadButton::DPadRight, "DPadRight", L"→" },
            ButtonNameEntry{ GamepadButton::Start, "Start", L"START" },
            ButtonNameEntry{ GamepadButton::Back, "Back", L"BACK" },
            ButtonNameEntry{ GamepadButton::LeftThumb, "LeftThumb", L"L3" },
            ButtonNameEntry{ GamepadButton::RightThumb, "RightThumb", L"R3" },
            ButtonNameEntry{ GamepadButton::LeftShoulder, "LeftShoulder", L"LB" },
            ButtonNameEntry{ GamepadButton::RightShoulder, "RightShoulder", L"RB" },
            ButtonNameEntry{ GamepadButton::A, "A", L"A" },
            ButtonNameEntry{ GamepadButton::B, "B", L"B" },
            ButtonNameEntry{ GamepadButton::X, "X", L"X" },
            ButtonNameEntry{ GamepadButton::Y, "Y", L"Y" },
            ButtonNameEntry{ GamepadButton::LeftTrigger, "LeftTrigger", L"LT" },
            ButtonNameEntry{ GamepadButton::RightTrigger, "RightTrigger", L"RT" },
            ButtonNameEntry{ GamepadButton::LeftStickUp, "LeftStickUp", L"L↑" },
            ButtonNameEntry{ GamepadButton::LeftStickDown, "LeftStickDown", L"L↓" },
            ButtonNameEntry{ GamepadButton::LeftStickLeft, "LeftStickLeft", L"L←" },
            ButtonNameEntry{ GamepadButton::LeftStickRight, "LeftStickRight", L"L→" },
        };

        // 1文字のキー表記（英字・数字）の置き場。表記は呼び出し側へポインタで返すので、静的に持っておく
        constexpr std::array<wchar_t[2], 36> kSingleCharNames = { {
            { L'A', 0 }, { L'B', 0 }, { L'C', 0 }, { L'D', 0 }, { L'E', 0 }, { L'F', 0 }, { L'G', 0 }, { L'H', 0 }, { L'I', 0 },
            { L'J', 0 }, { L'K', 0 }, { L'L', 0 }, { L'M', 0 }, { L'N', 0 }, { L'O', 0 }, { L'P', 0 }, { L'Q', 0 }, { L'R', 0 },
            { L'S', 0 }, { L'T', 0 }, { L'U', 0 }, { L'V', 0 }, { L'W', 0 }, { L'X', 0 }, { L'Y', 0 }, { L'Z', 0 }, { L'0', 0 },
            { L'1', 0 }, { L'2', 0 }, { L'3', 0 }, { L'4', 0 }, { L'5', 0 }, { L'6', 0 }, { L'7', 0 }, { L'8', 0 }, { L'9', 0 },
        } };

        constexpr std::array kFunctionKeyNames = { L"F1", L"F2", L"F3", L"F4", L"F5", L"F6", L"F7", L"F8", L"F9", L"F10", L"F11", L"F12" };

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
    const wchar_t* GetKeyDisplayName(KeyCode code) {
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

        return L"";
    }

    //----------------------------------------------------------------------------
    //! ゲームパッドのボタンを画面に出すときの表記を得ます。
    //----------------------------------------------------------------------------
    const wchar_t* GetGamepadButtonDisplayName(GamepadButton button) {
        for(const ButtonNameEntry& entry : kButtonNames) {
            if(entry.button == button)
                return entry.displayName;
        }
        return L"";
    }

}    // namespace Tsukino::Input

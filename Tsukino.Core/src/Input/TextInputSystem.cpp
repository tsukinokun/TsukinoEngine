//----------------------------------------------------------------------------
//! @file   TextInputSystem.cpp
//! @brief  文字入力と IME の受け口の実装
//----------------------------------------------------------------------------
#include <Tsukino/Core/Input/TextInputSystem.hpp>

#include <Tsukino/Core/Window.hpp>

#include <SDL3/SDL.h>

namespace {
    //----------------------------------------------------------------------------
    //! UTF-8 の文字列を、エンジンの文字描画が扱う wchar_t の文字列へ変換します。
    //! @param  [in] utf8 UTF-8 の文字列
    //! @return 変換した文字列。基本多言語面（U+FFFF まで）に無い文字と壊れたバイト列は捨てる
    //! @note   エンジンの文字描画は wchar_t 1 つを 1 文字として扱うため、
    //!         サロゲートペアになる文字（絵文字など）は描画できない。
    //----------------------------------------------------------------------------
    std::wstring Utf8ToWide(const std::string& utf8) {
        std::wstring result;
        result.reserve(utf8.size());

        std::size_t i = 0;
        while(i < utf8.size()) {
            const unsigned char lead = static_cast<unsigned char>(utf8[i]);

            // 先頭のバイトから、この文字のバイト数と最初の値を読む
            char32_t    codePoint = 0;
            std::size_t length    = 0;
            if(lead < 0x80) {
                codePoint = lead;
                length    = 1;
            } else if((lead & 0xE0) == 0xC0) {
                codePoint = lead & 0x1F;
                length    = 2;
            } else if((lead & 0xF0) == 0xE0) {
                codePoint = lead & 0x0F;
                length    = 3;
            } else if((lead & 0xF8) == 0xF0) {
                codePoint = lead & 0x07;
                length    = 4;
            } else {
                ++i;    // 先頭として不正なバイトは読み飛ばす
                continue;
            }

            // 続きのバイトを読む（途中で切れていたら、その文字は捨てる）
            bool valid = (i + length <= utf8.size());
            for(std::size_t k = 1; valid && k < length; ++k) {
                const unsigned char next = static_cast<unsigned char>(utf8[i + k]);
                if((next & 0xC0) != 0x80) {
                    valid = false;
                    break;
                }
                codePoint = (codePoint << 6) | (next & 0x3F);
            }

            if(!valid) {
                ++i;
                continue;
            }
            i += length;

            if(codePoint <= 0xFFFF && !(codePoint >= 0xD800 && codePoint <= 0xDFFF)) {
                result.push_back(static_cast<wchar_t>(codePoint));
            }
        }
        return result;
    }
}    // namespace

// 名前空間 : Tsukino::Input
namespace Tsukino::Input {
    //----------------------------------------------------------------------------
    //! 入力先のウィンドウを設定し、IME を切り離します。
    //----------------------------------------------------------------------------
    void TextInputSystem::Attach(Tsukino::Core::Window* window) {
        m_window = window;
        // IME が有効なままだと、キー入力が IME に奪われてゲーム操作が効かなくなる
        if(m_window && m_window->GetSDLWindow()) {
            SDL_StopTextInput(m_window->GetSDLWindow());
        }
    }

    //----------------------------------------------------------------------------
    //! 文字入力を開始し、IME を使える状態にします。
    //----------------------------------------------------------------------------
    void TextInputSystem::Begin(std::size_t maxLength) {
        m_active    = true;
        m_maxLength = maxLength;
        m_composition.clear();
        if(m_window && m_window->GetSDLWindow()) {
            SDL_StartTextInput(m_window->GetSDLWindow());
        }
    }

    //----------------------------------------------------------------------------
    //! 文字入力を終了し、IME を切り離します。入力済みの文字列は残ります。
    //----------------------------------------------------------------------------
    void TextInputSystem::End() {
        m_active = false;
        m_composition.clear();
        if(m_window && m_window->GetSDLWindow()) {
            SDL_StopTextInput(m_window->GetSDLWindow());
        }
    }

    //----------------------------------------------------------------------------
    //! 入力の出来事を処理します。Window の入力通知から、ほかの処理より先に呼びます。
    //----------------------------------------------------------------------------
    bool TextInputSystem::HandleEvent(const InputEvent& event) {
        if(!m_active)
            return false;

        switch(event.type) {
        //----------------------------------------------------------------------------
        // 確定した文字
        //----------------------------------------------------------------------------
        case InputEventType::TextInput:
            Append(Utf8ToWide(event.text));
            m_composition.clear();
            return true;

        //----------------------------------------------------------------------------
        // 変換中の文字列（空なら変換の終わり）
        //----------------------------------------------------------------------------
        case InputEventType::TextEditing:
            m_composition = Utf8ToWide(event.text);
            return true;

        //----------------------------------------------------------------------------
        // 1 文字消す。押しっぱなしの繰り返しでも消す。
        // 変換中は IME が自分で消すので、確定済みの文字列には触れない。
        // キーの押下としてはゲームへも渡す（false を返す）
        //----------------------------------------------------------------------------
        case InputEventType::KeyDown:
            if(event.key == KeyCode::Backspace && m_composition.empty() && !m_text.empty()) {
                m_text.pop_back();
            }
            return false;

        default: return false;
        }
    }

    //----------------------------------------------------------------------------
    //! 確定済みの文字列を置き換えます。
    //----------------------------------------------------------------------------
    void TextInputSystem::SetText(const std::wstring& text) {
        m_text.clear();
        Append(text);
    }

    //----------------------------------------------------------------------------
    //! 変換候補の窓を出す位置を設定します。
    //----------------------------------------------------------------------------
    void TextInputSystem::SetCandidatePosition(int x, int y) {
        if(!m_window || !m_window->GetSDLWindow())
            return;

        // SDL はウィンドウ座標（OS の拡大率で縮んだ座標）で受け取るので、ピクセルから戻す
        SDL_Window* window  = m_window->GetSDLWindow();
        const float density = SDL_GetWindowPixelDensity(window);
        const float scale   = (density > 0.0f) ? 1.0f / density : 1.0f;

        SDL_Rect area{};
        area.x = static_cast<int>(static_cast<float>(x) * scale);
        area.y = static_cast<int>(static_cast<float>(y) * scale);
        area.w = 1;
        area.h = 1;
        SDL_SetTextInputArea(window, &area, 0);
    }

    //----------------------------------------------------------------------------
    //! 確定した文字列を末尾へ追加します。上限を超える分とサロゲートペアは捨てます。
    //----------------------------------------------------------------------------
    void TextInputSystem::Append(const std::wstring& text) {
        for(wchar_t c : text) {
            // エンジンの文字描画は wchar_t 単位なので、サロゲートペアは描画できない
            if(c >= 0xD800 && c <= 0xDFFF)
                continue;
            // 制御文字（改行・タブなど）は入れない
            if(c < 0x20 || c == 0x7F)
                continue;
            if(m_maxLength != 0 && m_text.size() >= m_maxLength)
                break;
            m_text.push_back(c);
        }
    }
}    // namespace Tsukino::Input

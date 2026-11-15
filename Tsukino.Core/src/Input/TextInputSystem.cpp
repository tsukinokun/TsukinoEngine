//----------------------------------------------------------------------------
//! @file   TextInputSystem.cpp
//! @brief  文字入力と IME の受け口の実装
//----------------------------------------------------------------------------
#include <Tsukino/Core/Input/TextInputSystem.hpp>

#include <Tsukino/Core/Utf8.hpp>
#include <Tsukino/Core/Window.hpp>

#include <SDL3/SDL.h>

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
            Append(event.text);
            m_composition.clear();
            return true;

        //----------------------------------------------------------------------------
        // 変換中の文字列（空なら変換の終わり）
        //----------------------------------------------------------------------------
        case InputEventType::TextEditing:
            m_composition = event.text;
            return true;

        //----------------------------------------------------------------------------
        // 1 文字消す。押しっぱなしの繰り返しでも消す。
        // 変換中は IME が自分で消すので、確定済みの文字列には触れない。
        // キーの押下としてはゲームへも渡す（false を返す）
        //----------------------------------------------------------------------------
        case InputEventType::KeyDown:
            if(event.key == KeyCode::Backspace && m_composition.empty() && !m_text.empty()) {
                Tsukino::Core::Utf8::PopBack(m_text);
            }
            return false;

        default: return false;
        }
    }

    //----------------------------------------------------------------------------
    //! 確定済みの文字列を置き換えます。
    //----------------------------------------------------------------------------
    void TextInputSystem::SetText(const std::string& text) {
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
    //! 確定した文字列を末尾へ追加します。上限を超える分と制御文字は捨てます。
    //----------------------------------------------------------------------------
    void TextInputSystem::Append(const std::string& text) {
        std::size_t length = Tsukino::Core::Utf8::CountCodepoints(m_text);
        std::size_t index  = 0;
        while(index < text.size()) {
            const char32_t c = Tsukino::Core::Utf8::DecodeNext(text, index);
            // 制御文字（改行・タブなど）と、読めなかったバイト列は入れない
            if(c < 0x20 || c == 0x7F || c == Tsukino::Core::Utf8::kReplacementCharacter)
                continue;
            if(m_maxLength != 0 && length >= m_maxLength)
                break;
            Tsukino::Core::Utf8::Append(m_text, c);
            ++length;
        }
    }
}    // namespace Tsukino::Input

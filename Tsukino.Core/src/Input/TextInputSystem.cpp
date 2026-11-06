//----------------------------------------------------------------------------
//! @file   TextInputSystem.cpp
//! @brief  文字入力と IME の受け口の実装
//----------------------------------------------------------------------------
#include <Tsukino/Core/Input/TextInputSystem.hpp>

#include <imm.h>

// 名前空間 : Tsukino::Input
namespace Tsukino::Input {
    //----------------------------------------------------------------------------
    //! 入力先のウィンドウを設定し、IME を切り離します。
    //----------------------------------------------------------------------------
    void TextInputSystem::Attach(HWND hwnd) {
        m_hwnd = hwnd;
        // IME が有効なままだと、キー入力が VK_PROCESSKEY に化けてゲーム操作が効かなくなる
        ImmAssociateContextEx(m_hwnd, nullptr, 0);
    }

    //----------------------------------------------------------------------------
    //! 文字入力を開始し、IME を使える状態にします。
    //----------------------------------------------------------------------------
    void TextInputSystem::Begin(std::size_t maxLength) {
        m_active    = true;
        m_maxLength = maxLength;
        m_composition.clear();
        ImmAssociateContextEx(m_hwnd, nullptr, IACE_DEFAULT);
    }

    //----------------------------------------------------------------------------
    //! 文字入力を終了し、IME を切り離します。入力済みの文字列は残ります。
    //----------------------------------------------------------------------------
    void TextInputSystem::End() {
        m_active = false;
        m_composition.clear();
        ImmAssociateContextEx(m_hwnd, nullptr, 0);
    }

    //----------------------------------------------------------------------------
    //! ウィンドウメッセージを処理します。Window のメッセージフィルタから呼びます。
    //----------------------------------------------------------------------------
    bool TextInputSystem::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        if(!m_active)
            return false;

        switch(msg) {
        //----------------------------------------------------------------------------
        // 既定の処理へ渡すと OS 標準の変換窓が出るため、変換の開始と終了はここで止める
        //----------------------------------------------------------------------------
        case WM_IME_STARTCOMPOSITION:
            return true;

        case WM_IME_ENDCOMPOSITION:
            m_composition.clear();
            return true;

        case WM_IME_COMPOSITION:
            // 確定文字は WM_CHAR を待たずここで読む。ImmGetCompositionStringW は
            // ウィンドウの文字コードに関係なく UTF-16 を返す
            if(lParam & GCS_RESULTSTR)
                Append(ReadCompositionString(GCS_RESULTSTR));
            if(lParam & GCS_COMPSTR)
                m_composition = ReadCompositionString(GCS_COMPSTR);
            return true;

        case WM_CHAR: {
            const wchar_t c = static_cast<wchar_t>(wParam);
            if(c == L'\b') {
                if(!m_text.empty())
                    m_text.pop_back();
            } else if(c >= 0x20 && c != 0x7F) {
                Append(std::wstring(1, c));
            }
            return true;
        }

        default:
            return false;
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
        HIMC context = ImmGetContext(m_hwnd);
        if(context == nullptr)
            return;

        CANDIDATEFORM form = {};
        form.dwStyle       = CFS_CANDIDATEPOS;
        form.ptCurrentPos  = {x, y};
        ImmSetCandidateWindow(context, &form);
        ImmReleaseContext(m_hwnd, context);
    }

    //----------------------------------------------------------------------------
    //! 確定した文字列を末尾へ追加します。上限を超える分とサロゲートペアは捨てます。
    //----------------------------------------------------------------------------
    void TextInputSystem::Append(const std::wstring& text) {
        for(wchar_t c : text) {
            // エンジンの文字描画は wchar_t 単位なので、サロゲートペアは描画できない
            if(c >= 0xD800 && c <= 0xDFFF)
                continue;
            if(m_maxLength != 0 && m_text.size() >= m_maxLength)
                break;
            m_text.push_back(c);
        }
    }

    //----------------------------------------------------------------------------
    //! IME から文字列を読みます。
    //----------------------------------------------------------------------------
    std::wstring TextInputSystem::ReadCompositionString(DWORD index) const {
        std::wstring result;

        HIMC context = ImmGetContext(m_hwnd);
        if(context == nullptr)
            return result;

        const LONG bytes = ImmGetCompositionStringW(context, index, nullptr, 0);
        if(bytes > 0) {
            result.resize(static_cast<std::size_t>(bytes) / sizeof(wchar_t));
            ImmGetCompositionStringW(context, index, result.data(), static_cast<DWORD>(bytes));
        }
        ImmReleaseContext(m_hwnd, context);
        return result;
    }
}    // namespace Tsukino::Input

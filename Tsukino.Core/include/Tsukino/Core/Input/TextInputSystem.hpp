//----------------------------------------------------------------------------
//! @file   TextInputSystem.hpp
//! @brief  文字入力と IME の受け口
//! @detail キーの押下ではなく「入力された文字」を受け取ります。名前入力のように、
//!         日本語の変換を含む文字列をゲームへ渡すときに使います。
//!         入力中以外は IME を切り離し、ゲーム操作のキーが IME に奪われないようにします。
//----------------------------------------------------------------------------
#pragma once

#include <Tsukino/Core/WindowsLean.hpp>

#include <cstddef>
#include <string>

// 名前空間 : Tsukino::Input
namespace Tsukino::Input {
    class TextInputSystem {
    public:

        //! 入力先のウィンドウを設定し、IME を切り離します。
        //! @param  [in] hwnd 入力を受け取るウィンドウ
        void Attach(HWND hwnd);

        //! 文字入力を開始し、IME を使える状態にします。
        //! @param  [in] maxLength 入力できる文字数の上限（0 で制限なし）
        void Begin(std::size_t maxLength = 0);

        //! 文字入力を終了し、IME を切り離します。入力済みの文字列は残ります。
        void End();

        //! 文字入力の最中かを返します。
        //! @return 入力中なら true
        bool IsActive() const { return m_active; }

        //! ウィンドウメッセージを処理します。Window のメッセージフィルタから呼びます。
        //! @param  [in] msg    メッセージの種類
        //! @param  [in] wParam メッセージの引数
        //! @param  [in] lParam メッセージの引数
        //! @return メッセージを処理し、既定の処理へ渡さないなら true
        bool HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        //! 確定済みの文字列を返します。
        //! @return 確定済みの文字列
        const std::wstring& GetText() const { return m_text; }

        //! 確定済みの文字列を置き換えます。
        //! @param  [in] text 新しい文字列
        void SetText(const std::wstring& text);

        //! 変換中の文字列を返します。ゲーム側で確定済みの文字列に続けて表示します。
        //! @return 変換中の文字列。変換中でなければ空
        const std::wstring& GetComposition() const { return m_composition; }

        //! 変換候補の窓を出す位置を設定します。
        //! @param  [in] x ウィンドウのクライアント座標の X
        //! @param  [in] y ウィンドウのクライアント座標の Y
        void SetCandidatePosition(int x, int y);

    private:

        //! 確定した文字列を末尾へ追加します。上限を超える分とサロゲートペアは捨てます。
        //! @param  [in] text 追加する文字列
        void Append(const std::wstring& text);

        //! IME から文字列を読みます。
        //! @param  [in] index 読む文字列の種類（GCS_COMPSTR または GCS_RESULTSTR）
        //! @return 読み出した文字列
        std::wstring ReadCompositionString(DWORD index) const;

    private:
        HWND         m_hwnd      = nullptr;    // 入力先のウィンドウ
        bool         m_active    = false;      // 入力中かどうか
        std::size_t  m_maxLength = 0;          // 文字数の上限（0 で制限なし）
        std::wstring m_text;                   // 確定済みの文字列
        std::wstring m_composition;            // 変換中の文字列
    };
}    // namespace Tsukino::Input

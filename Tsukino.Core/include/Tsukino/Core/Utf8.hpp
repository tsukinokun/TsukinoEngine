//----------------------------------------------------------------------------
//! @file   Utf8.hpp
//! @brief  UTF-8 の文字列を1文字（コードポイント）ずつ扱う関数
//! @detail エンジンの文字列はすべて UTF-8 の std::string です（画面に出す文字・パス・入力された文字）。
//!         文字を描くときや文字数を数えるときは、ここの関数でコードポイントへ分けます。
//----------------------------------------------------------------------------
#pragma once

#include <cstddef>
#include <string>
#include <string_view>

// 名前空間 : Tsukino::Core::Utf8
namespace Tsukino::Core::Utf8 {

    //! 読めなかったバイト列の代わりに返す文字（U+FFFD）
    inline constexpr char32_t kReplacementCharacter = 0xFFFD;

    //! 1文字を読み、位置を次の文字へ進めます。
    //! @param  [in]     text  UTF-8 の文字列
    //! @param  [in,out] index 読む位置（バイト）。読んだ分だけ進む
    //! @return 読んだ文字。壊れたバイト列は1バイト進めて kReplacementCharacter を返す
    //! @note   index が末尾に達していれば 0 を返し、index は動かさない
    [[nodiscard]]
    char32_t DecodeNext(std::string_view text, std::size_t& index) noexcept;

    //! 1文字を UTF-8 にして末尾へ足します。
    //! @param  [in,out] text      足す先
    //! @param  [in]     codepoint 足す文字（サロゲートと範囲外は kReplacementCharacter にする）
    void Append(std::string& text, char32_t codepoint);

    //! 文字数（コードポイントの数）を数えます。
    //! @param  [in] text UTF-8 の文字列
    //! @return 文字数
    [[nodiscard]]
    std::size_t CountCodepoints(std::string_view text) noexcept;

    //! 末尾の1文字を消します（文字の途中で切らない）。
    //! @param  [in,out] text UTF-8 の文字列。空なら何もしない
    void PopBack(std::string& text) noexcept;

}    // namespace Tsukino::Core::Utf8

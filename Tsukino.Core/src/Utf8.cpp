//----------------------------------------------------------------------------
//! @file   Utf8.cpp
//! @brief  UTF-8 の文字列を1文字（コードポイント）ずつ扱う関数の実装
//----------------------------------------------------------------------------
#include <Tsukino/Core/Utf8.hpp>

// 名前空間 : Tsukino::Core::Utf8
namespace Tsukino::Core::Utf8 {
    namespace {
        //! 続きのバイト（10xxxxxx）かを返します。
        bool IsContinuation(unsigned char byte) noexcept {
            return (byte & 0xC0) == 0x80;
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! 1文字を読み、位置を次の文字へ進めます。
    //----------------------------------------------------------------------------
    char32_t DecodeNext(std::string_view text, std::size_t& index) noexcept {
        if(index >= text.size())
            return 0;

        const unsigned char lead = static_cast<unsigned char>(text[index]);

        // 先頭のバイトから、この文字のバイト数・最初の値・その長さで表せる最小の値を決める
        std::size_t length  = 0;
        char32_t    value   = 0;
        char32_t    minimum = 0;
        if(lead < 0x80) {
            ++index;
            return lead;
        } else if((lead & 0xE0) == 0xC0) {
            length  = 2;
            value   = lead & 0x1F;
            minimum = 0x80;
        } else if((lead & 0xF0) == 0xE0) {
            length  = 3;
            value   = lead & 0x0F;
            minimum = 0x800;
        } else if((lead & 0xF8) == 0xF0) {
            length  = 4;
            value   = lead & 0x07;
            minimum = 0x10000;
        } else {
            ++index;    // 先頭として不正なバイト
            return kReplacementCharacter;
        }

        if(index + length > text.size()) {
            ++index;    // 途中で切れている
            return kReplacementCharacter;
        }
        for(std::size_t k = 1; k < length; ++k) {
            const unsigned char next = static_cast<unsigned char>(text[index + k]);
            if(!IsContinuation(next)) {
                ++index;
                return kReplacementCharacter;
            }
            value = (value << 6) | (next & 0x3F);
        }

        // 長すぎる表し方・サロゲート・範囲外は読めない文字とする
        if(value < minimum || (value >= 0xD800 && value <= 0xDFFF) || value > 0x10FFFF) {
            ++index;
            return kReplacementCharacter;
        }

        index += length;
        return value;
    }

    //----------------------------------------------------------------------------
    //! 1文字を UTF-8 にして末尾へ足します。
    //----------------------------------------------------------------------------
    void Append(std::string& text, char32_t codepoint) {
        if((codepoint >= 0xD800 && codepoint <= 0xDFFF) || codepoint > 0x10FFFF)
            codepoint = kReplacementCharacter;

        if(codepoint < 0x80) {
            text.push_back(static_cast<char>(codepoint));
        } else if(codepoint < 0x800) {
            text.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if(codepoint < 0x10000) {
            text.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            text.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            text.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
            text.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            text.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
    }

    //----------------------------------------------------------------------------
    //! 文字数（コードポイントの数）を数えます。
    //----------------------------------------------------------------------------
    std::size_t CountCodepoints(std::string_view text) noexcept {
        std::size_t count = 0;
        std::size_t index = 0;
        while(index < text.size()) {
            (void)DecodeNext(text, index);
            ++count;
        }
        return count;
    }

    //----------------------------------------------------------------------------
    //! 末尾の1文字を消します（文字の途中で切らない）。
    //----------------------------------------------------------------------------
    void PopBack(std::string& text) noexcept {
        if(text.empty())
            return;
        // 続きのバイトを飛ばして、その文字の先頭まで戻る（壊れた並びでも最大4バイトで止める）
        std::size_t start = text.size() - 1;
        for(int k = 0; k < 3 && start > 0 && IsContinuation(static_cast<unsigned char>(text[start])); ++k) --start;
        text.erase(start);
    }

}    // namespace Tsukino::Core::Utf8

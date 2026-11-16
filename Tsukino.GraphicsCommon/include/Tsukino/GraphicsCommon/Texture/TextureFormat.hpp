//----------------------------------------------------------------------------
//! @file   TextureFormat.hpp
//! @brief  テクスチャのピクセルフォーマットの種類
//! @detail 描画 API に依存しない、エンジン独自のフォーマットの列挙です。
//!         各 API のフォーマットへの変換は、使う側のモジュールの中で行います。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

// 名前空間 : Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {

    //------------------------------------------------------------------------
    //! テクスチャのピクセルフォーマットです。
    //! @note  名前の末尾の _sRGB は、色を sRGB として読む（サンプリング時に線形へ戻す）ことを表す。
    //!        BC はブロック圧縮（4x4 ピクセルを1ブロックにまとめる形式）
    //------------------------------------------------------------------------
    enum class TextureFormat : u8 {
        Unknown,       //!< 不明（圧縮の規則では「圧縮しない」の意味）
        R8,            //!< 8bit × 1ch
        RG8,           //!< 8bit × 2ch
        RGBA8,         //!< 8bit × 4ch（線形）
        RGBA8_sRGB,    //!< 8bit × 4ch（sRGB）
        BGRA8,         //!< 8bit × 4ch・BGRA の並び（線形）
        BGRA8_sRGB,    //!< 8bit × 4ch・BGRA の並び（sRGB）
        R16,           //!< 16bit 整数 × 1ch
        RGBA16,        //!< 16bit 整数 × 4ch
        R16F,          //!< 16bit 浮動小数 × 1ch
        RGBA16F,       //!< 16bit 浮動小数 × 4ch
        R32F,          //!< 32bit 浮動小数 × 1ch
        RGBA32F,       //!< 32bit 浮動小数 × 4ch
        RGB10A2,       //!< 10bit × 3ch ＋ 2bit
        BC1,           //!< ブロック圧縮 BC1（線形。アルファは 1bit）
        BC1_sRGB,      //!< ブロック圧縮 BC1（sRGB）
        BC3,           //!< ブロック圧縮 BC3（線形。アルファ付き）
        BC3_sRGB,      //!< ブロック圧縮 BC3（sRGB）
        BC4,           //!< ブロック圧縮 BC4（1ch）
        BC5,           //!< ブロック圧縮 BC5（2ch。法線マップ向け）
        BC7,           //!< ブロック圧縮 BC7（線形。高品質）
        BC7_sRGB,      //!< ブロック圧縮 BC7（sRGB）
        ASTC4x4,       //!< ASTC 4×4（線形。スマホ向けの高品質な圧縮。BC3 / BC7 の代わり）
        ASTC4x4_sRGB,  //!< ASTC 4×4（sRGB）
        ASTC6x6,       //!< ASTC 6×6（線形。スマホ向けの軽い圧縮。BC1 の代わり）
        ASTC6x6_sRGB,  //!< ASTC 6×6（sRGB）
        Count
    };

}    // namespace Tsukino::GraphicsCommon

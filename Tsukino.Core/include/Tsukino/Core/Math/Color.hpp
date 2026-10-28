//----------------------------------------------------------------------------
//! @file   Color.hpp
//! @brief  色空間の変換（見た目の sRGB と、描画に使う線形の色の行き来）
//! @detail エンジンが受け取る色（文字・スプライト・ライト・フォグ・粒子など）はすべて線形の値です。
//!         描画先は sRGB のバッファなので、書き込むときに線形から sRGB へ自動で変換されます。
//!         デザインツールや画像の上で選んだ「見た目どおりの色」は sRGB の値なので、
//!         そのまま渡すと明るく薄く見えます。ここの関数で線形に変換してから渡してください。
//----------------------------------------------------------------------------
#pragma once
#include <hlsl++.h>

#include <algorithm>
#include <cmath>

// 名前空間 : Tsukino::Core::Math::Color
namespace Tsukino::Core::Math::Color {

    //! sRGB の値（0〜1）を線形の値に変換します。
    //! @param  [in] c sRGB の値（0〜1）
    //! @return 線形の値（0〜1）
    [[nodiscard]] inline float SrgbToLinear(float c) {
        c = std::clamp(c, 0.0f, 1.0f);
        // 暗いところは直線、それ以外は 2.4 乗の曲線（sRGB の規格どおり）
        return (c <= 0.04045f) ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
    }

    //! 線形の値（0〜1）を sRGB の値に変換します。
    //! @param  [in] c 線形の値（0〜1）
    //! @return sRGB の値（0〜1）
    [[nodiscard]] inline float LinearToSrgb(float c) {
        c = std::clamp(c, 0.0f, 1.0f);
        return (c <= 0.0031308f) ? c * 12.92f : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
    }

    //! sRGB の色を線形の色に変換します。
    //! @param  [in] color sRGB の色（各要素 0〜1）
    //! @return 線形の色
    [[nodiscard]] inline hlslpp::float3 SrgbToLinear(const hlslpp::float3& color) {
        return hlslpp::float3(SrgbToLinear(float(color.x)), SrgbToLinear(float(color.y)), SrgbToLinear(float(color.z)));
    }

    //! sRGB の色を線形の色に変換します（アルファは不透明度なので変換しない）。
    //! @param  [in] color sRGB の色（各要素 0〜1）
    //! @return 線形の色
    [[nodiscard]] inline hlslpp::float4 SrgbToLinear(const hlslpp::float4& color) {
        return hlslpp::float4(SrgbToLinear(float(color.x)), SrgbToLinear(float(color.y)), SrgbToLinear(float(color.z)), float(color.w));
    }

    //! 線形の色を sRGB の色に変換します（アルファは変換しない）。
    //! @param  [in] color 線形の色（各要素 0〜1）
    //! @return sRGB の色
    [[nodiscard]] inline hlslpp::float4 LinearToSrgb(const hlslpp::float4& color) {
        return hlslpp::float4(LinearToSrgb(float(color.x)), LinearToSrgb(float(color.y)), LinearToSrgb(float(color.z)), float(color.w));
    }

    //! 0〜255 の見た目の色（デザインツールの値）から、線形の色を作ります。
    //! @param  [in] r     赤（0〜255）
    //! @param  [in] g     緑（0〜255）
    //! @param  [in] b     青（0〜255）
    //! @param  [in] alpha 不透明度（0〜1）
    //! @return 線形の色
    [[nodiscard]] inline hlslpp::float4 FromSrgb8(int r, int g, int b, float alpha = 1.0f) {
        return SrgbToLinear(hlslpp::float4(static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f, alpha));
    }

}    // namespace Tsukino::Core::Math::Color

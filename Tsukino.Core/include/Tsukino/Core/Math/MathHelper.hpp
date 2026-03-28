//--------------------------------------------------------------
//! @file       MathHelper.hpp
//! @brief      数学関連の定数や関数の宣言
//! @author     山﨑愛
//--------------------------------------------------------------
#pragma once
// 名前空間 : Tsukino::Math
namespace Tsukino::Core::Math {
    // --- 定数 ---
    static constexpr float PI      = 3.1415926535f;
    static constexpr float TWO_PI  = PI * 2.0f;
    static constexpr float HALF_PI = PI * 0.5f;
    static constexpr float Deg2Rad = PI / 180.0f;
    static constexpr float Rad2Deg = 180.0f / PI;

    //--------------------------------------------------------------
    //! @brief 度数法を弧度法に変換する関数
    //--------------------------------------------------------------
    inline float ToRadians(float degrees) {
        return degrees * Deg2Rad;
    }

    //--------------------------------------------------------------
    //! @brief 弧度法を度数法に変換する関数
    //--------------------------------------------------------------
    inline float ToDegrees(float radians) {
        return radians * Rad2Deg;
    }
}    // namespace Tsukino::Math

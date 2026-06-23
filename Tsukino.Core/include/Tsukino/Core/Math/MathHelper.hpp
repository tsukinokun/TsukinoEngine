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
    inline [[nodiscard]]
    float ToRadians(float degrees) {
        return degrees * Deg2Rad;
    }

    //--------------------------------------------------------------
    //! @brief 弧度法を度数法に変換する関数
    //--------------------------------------------------------------
    inline [[nodiscard]]
    float ToDegrees(float radians) {
        return radians * Rad2Deg;
    }

    //--------------------------------------------------------------
    //! @brief 2つの矩形（AABB）が重なっているかを判定する
    //! @param [in] min1 矩形1の左上座標
    //! @param [in] size1 矩形1のサイズ
    //! @param [in] min2 矩形2の左上座標
    //! @param [in] size2 矩形2のサイズ
    //! @return 重なっている場合はtrue、そうでない場合はfalse
    //--------------------------------------------------------------
    inline [[nodiscard]]
    bool Intersects(const hlslpp::float2& min1, const hlslpp::float2& size1, const hlslpp::float2& min2, const hlslpp::float2& size2) {
        return (min1.x < min2.x + size2.x) && (min1.x + size1.x > min2.x) && (min1.y < min2.y + size2.y) && (min1.y + size1.y > min2.y);
    }
}    // namespace Tsukino::Core::Math

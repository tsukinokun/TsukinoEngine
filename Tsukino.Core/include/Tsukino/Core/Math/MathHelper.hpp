//--------------------------------------------------------------
//! @file       MathHelper.hpp
//! @brief      数学関連の定数や関数の宣言
//! @author     山﨑愛
//--------------------------------------------------------------
#pragma once
#include <hlsl++.h>
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

    //--------------------------------------------------------------
    //! @brief 中心点とサイズから、点が矩形内にあるか判定する関数
    //! @param [in] point 判定する点の座標
    //! @param [in] center 矩形の中心座標
    //! @param [in] size 矩形のサイズ
    //! @return 点が矩形内にある場合はtrue、そうでない場合はfalse
    //--------------------------------------------------------------
    inline [[nodiscard]]
    bool IsPointInRect(const hlslpp::float2& point, const hlslpp::float2& center, const hlslpp::float2& size) {
        hlslpp::float2 halfSize = size * 0.5f;
        return (point.x >= center.x - halfSize.x) && (point.x <= center.x + halfSize.x) && (point.y >= center.y - halfSize.y)
               && (point.y <= center.y + halfSize.y);
    }

    //--------------------------------------------------------------
    //! @brief  最短経路を保証したslerp
    //! @note   hlslpp::slerpは二重被覆（qと-qが同じ回転を表す性質）を考慮しないため、
    //!         現在の向きと目標の向きの内積が負（=180°境界をまたいでいる）の場合に
    //!         最短経路ではなく反対周りへ補間してしまうことがある。呼び出し前に
    //!         内積の符号をチェックし、必要ならq1側を符号反転してからhlslpp::slerpへ
    //!         渡すことでこれを避ける
    //! @param  [in] q0 補間元の向き
    //! @param  [in] q1 補間先の向き
    //! @param  [in] t  補間係数（0〜1）
    //! @return q0からq1への、常に近い方の経路で補間した向き
    //--------------------------------------------------------------
    inline [[nodiscard]]
    hlslpp::quaternion SlerpShortestPath(const hlslpp::quaternion& q0, const hlslpp::quaternion& q1, float t) {
        float rotDot = q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w;
        hlslpp::quaternion q1Shortest = (rotDot < 0.0f) ? hlslpp::quaternion(-q1.x, -q1.y, -q1.z, -q1.w) : q1;
        return hlslpp::slerp(q0, q1Shortest, t);
    }
}    // namespace Tsukino::Core::Math

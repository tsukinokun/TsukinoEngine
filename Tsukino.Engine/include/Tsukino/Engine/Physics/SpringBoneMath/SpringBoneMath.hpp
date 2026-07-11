//--------------------------------------------------------------
//! @file   SpringBoneMath.hpp
//! @brief  揺れ物(SpringBone)物理で使う補助演算ファイル
//! @author 山﨑 愛
//--------------------------------------------------------------
#pragma once
#include <hlsl++.h>
#include <cmath>

// 名前空間 Tsukino::Physics
namespace Tsukino::Physics {

    //--------------------------------------------------------------
    //! @brief  正規化済み2ベクトル間の最短回転クォータニオンを求める
    //--------------------------------------------------------------
    inline hlslpp::quaternion QuatFromToRotation(const hlslpp::float3& from, const hlslpp::float3& to) {
        float d = hlslpp::dot(from, to);

        // ほぼ同じ方向 → 回転なし
        if(d > 0.999999f) {
            return hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
        }

        // ほぼ正反対 → fromに垂直な軸で180度回転
        if(d < -0.999999f) {
            hlslpp::float3 axis = hlslpp::cross(hlslpp::float3(1.0f, 0.0f, 0.0f), from);
            if(float(hlslpp::length(axis)) < 1e-6f) {
                axis = hlslpp::cross(hlslpp::float3(0.0f, 1.0f, 0.0f), from);
            }
            axis = hlslpp::normalize(axis);
            return hlslpp::quaternion(axis.x, axis.y, axis.z, 0.0f);
        }

        hlslpp::float3 axis = hlslpp::cross(from, to);
        float          s    = std::sqrt((1.0f + d) * 2.0f);
        float          invs = 1.0f / s;

        return hlslpp::normalize(hlslpp::quaternion(axis.x * invs, axis.y * invs, axis.z * invs, s * 0.5f));
    }

    //--------------------------------------------------------------
    //! @brief  方向ベクトルの簡易球面補間（角度制限のクランプ用途）
    //! @note   厳密なslerpではないが、角度制限の用途では十分な近似
    //--------------------------------------------------------------
    inline hlslpp::float3 NLerpDirection(const hlslpp::float3& from, const hlslpp::float3& to, float t) {
        hlslpp::float3 blended = from + (to - from) * t;
        float          len     = float(hlslpp::length(blended));
        if(len < 1e-6f) {
            return to;
        }
        return blended / len;
    }

}    // namespace Tsukino::Physics

//----------------------------------------------------------------------------
//! @file   Spring.hpp
//! @brief  減衰ばね（減衰調和振動子）を時間で進める関数
//! @detail カメラの追従・揺れ、UI の値の追従など「目標へ引き寄せられながら、行き過ぎて揺り戻す」動きに使います。
//!         運動方程式の解析解で deltaTime ぶん厳密に進めるので、フレームレートが変わっても同じ動きになり、
//!         deltaTime が大きくても発散しません（陰的オイラーで高い振動数を回すと、計算上の減衰で振れ幅が潰れ、
//!         しかもフレームレートで振れ幅が変わってしまいます）。
//----------------------------------------------------------------------------
#pragma once
#include <algorithm>
#include <cmath>

// 名前空間 : Tsukino::Core::Math::Spring
namespace Tsukino::Core::Math::Spring {

    //! 円周率の2倍（Hz から角周波数への変換に使う）
    inline constexpr float kTwoPi = 6.28318531f;

    //! 減衰ばねを1ステップ進めます。
    //! @tparam T float か、スカラー倍・加減算ができるベクトル型（hlslpp::float2 / float3 など）
    //! @param  [in,out] position  現在位置
    //! @param  [in,out] velocity  現在速度
    //! @param  [in]     target    引き寄せられる先（このステップの間は動かないものとして扱う）
    //! @param  [in]     frequency 固有振動数（Hz）
    //! @param  [in]     damping   減衰比（1 未満で行き過ぎて揺り戻す、1 で行き過ぎ無しの最短、1 より大きいとゆっくり収まる）
    //! @param  [in]     deltaTime 経過秒（0 以下なら何もしない）
    template <typename T>
    void Step(T& position, T& velocity, const T& target, float frequency, float damping, float deltaTime) {
        if(deltaTime <= 0.0f)
            return;

        const float omega = kTwoPi * std::max(frequency, 1.0e-3f);    // Hz → 角周波数(rad/s)
        const float zeta  = std::max(damping, 0.0f);

        // 目標からのずれとして解く
        const T x0 = position - target;
        const T v0 = velocity;

        T x;
        T v;

        if(zeta < 0.9999f) {
            // 減衰振動（行き過ぎて何度か揺り戻す）
            const float decay    = zeta * omega;
            const float omegaD   = omega * std::sqrt(1.0f - zeta * zeta);
            const float envelope = std::exp(-decay * deltaTime);
            const float cosTerm  = std::cos(omegaD * deltaTime);
            const float sinTerm  = std::sin(omegaD * deltaTime);

            x = (x0 * cosTerm + (v0 + x0 * decay) * (sinTerm / omegaD)) * envelope;
            v = (v0 * cosTerm - (x0 * (omega * omega) + v0 * decay) * (sinTerm / omegaD)) * envelope;
        } else if(zeta <= 1.0001f) {
            // 臨界減衰（行き過ぎずに最短で収まる）
            const float envelope = std::exp(-omega * deltaTime);
            const T     k        = v0 + x0 * omega;

            x = (x0 + k * deltaTime) * envelope;
            v = (v0 - k * (omega * deltaTime)) * envelope;
        } else {
            // 過減衰（行き過ぎずにゆっくり収まる）
            const float root = std::sqrt(zeta * zeta - 1.0f);
            const float r1   = -omega * (zeta - root);
            const float r2   = -omega * (zeta + root);
            const T     c1   = (v0 - x0 * r2) / (r1 - r2);
            const T     c2   = x0 - c1;
            const float e1   = std::exp(r1 * deltaTime);
            const float e2   = std::exp(r2 * deltaTime);

            x = c1 * e1 + c2 * e2;
            v = c1 * (r1 * e1) + c2 * (r2 * e2);
        }

        position = target + x;
        velocity = v;
    }

}    // namespace Tsukino::Core::Math::Spring

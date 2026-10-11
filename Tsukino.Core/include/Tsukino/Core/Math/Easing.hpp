//----------------------------------------------------------------------------
//! @file   Easing.hpp
//! @brief  イージング関数（0〜1 の進み具合を、動きの緩急を付けた値に変換する）
//! @detail UI のトゥイーン（UITweenComponent）や、ゲーム側の演出で使います。
//!         どの関数も t = 0 で 0、t = 1 で 1 を返します（Punch だけは両端とも 0）。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/MathHelper.hpp>

#include <algorithm>
#include <cmath>

// 名前空間 : Tsukino::Core::Math::Easing
namespace Tsukino::Core::Math::Easing {

    //! イージングの種類です。
    enum class EaseType {
        Linear,        // 一定の速さ
        InQuad,        // ゆっくり始まる（2次）
        OutQuad,       // ゆっくり終わる（2次）
        InOutQuad,     // ゆっくり始まりゆっくり終わる（2次）
        InCubic,       // ゆっくり始まる（3次）
        OutCubic,      // ゆっくり終わる（3次）
        InOutCubic,    // ゆっくり始まりゆっくり終わる（3次）
        OutBack,       // 少し行き過ぎてから戻る（ポップ）
        OutElastic,    // ばねのように揺れながら止まる
        OutBounce,     // 床で跳ねるように止まる
        InOutSine,     // 正弦波でなめらかに往復する（呼吸の動き向け）
        Punch,         // 0 から行き過ぎて揺れながら 0 に戻る（弾む動き向け。終わりの値は 0）
    };

    //! OutBack の既定の行き過ぎの強さです。
    static constexpr float kDefaultOvershoot = 1.70158f;

    //! 進み具合を 0〜1 に収めます。
    //! @param  [in] t 進み具合
    //! @return 0〜1 に収めた値
    [[nodiscard]] inline float Clamp01(float t) {
        return std::clamp(t, 0.0f, 1.0f);
    }

    //! 一定の速さで進む値を返します。
    //! @param  [in] t 進み具合（0〜1）
    //! @return 変換した値
    [[nodiscard]] inline float Linear(float t) {
        return Clamp01(t);
    }

    //! ゆっくり始まる値（2次）を返します。
    //! @param  [in] t 進み具合（0〜1）
    //! @return 変換した値
    [[nodiscard]] inline float InQuad(float t) {
        t = Clamp01(t);
        return t * t;
    }

    //! ゆっくり終わる値（2次）を返します。
    //! @param  [in] t 進み具合（0〜1）
    //! @return 変換した値
    [[nodiscard]] inline float OutQuad(float t) {
        t = Clamp01(t);
        return 1.0f - (1.0f - t) * (1.0f - t);
    }

    //! ゆっくり始まりゆっくり終わる値（2次）を返します。
    //! @param  [in] t 進み具合（0〜1）
    //! @return 変換した値
    [[nodiscard]] inline float InOutQuad(float t) {
        t = Clamp01(t);
        return (t < 0.5f) ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
    }

    //! ゆっくり始まる値（3次）を返します。
    //! @param  [in] t 進み具合（0〜1）
    //! @return 変換した値
    [[nodiscard]] inline float InCubic(float t) {
        t = Clamp01(t);
        return t * t * t;
    }

    //! ゆっくり終わる値（3次）を返します。
    //! @param  [in] t 進み具合（0〜1）
    //! @return 変換した値
    [[nodiscard]] inline float OutCubic(float t) {
        t             = Clamp01(t);
        const float u = 1.0f - t;
        return 1.0f - u * u * u;
    }

    //! ゆっくり始まりゆっくり終わる値（3次）を返します。
    //! @param  [in] t 進み具合（0〜1）
    //! @return 変換した値
    [[nodiscard]] inline float InOutCubic(float t) {
        t = Clamp01(t);
        if(t < 0.5f)
            return 4.0f * t * t * t;
        const float u = 1.0f - t;
        return 1.0f - 4.0f * u * u * u;
    }

    //! 少し行き過ぎてから 1 に戻る値を返します。
    //! @param  [in] t         進み具合（0〜1）
    //! @param  [in] overshoot 行き過ぎの強さ（0 で行き過ぎなし。大きいほど行き過ぎる）
    //! @return 変換した値（途中で 1 を超える）
    [[nodiscard]] inline float OutBack(float t, float overshoot = kDefaultOvershoot) {
        t             = Clamp01(t);
        const float u = t - 1.0f;
        return 1.0f + (overshoot + 1.0f) * u * u * u + overshoot * u * u;
    }

    //! ばねのように揺れながら 1 に止まる値を返します。
    //! @param  [in] t 進み具合（0〜1）
    //! @return 変換した値（途中で 1 の上下に揺れる）
    [[nodiscard]] inline float OutElastic(float t) {
        t = Clamp01(t);
        if(t <= 0.0f || t >= 1.0f)
            return t;
        constexpr float kPeriod = TWO_PI / 3.0f;
        return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * kPeriod) + 1.0f;
    }

    //! 床で跳ねるように 1 に止まる値を返します。
    //! @param  [in] t 進み具合（0〜1）
    //! @return 変換した値
    [[nodiscard]] inline float OutBounce(float t) {
        t                  = Clamp01(t);
        constexpr float n1 = 7.5625f;
        constexpr float d1 = 2.75f;
        if(t < 1.0f / d1)
            return n1 * t * t;
        if(t < 2.0f / d1) {
            t -= 1.5f / d1;
            return n1 * t * t + 0.75f;
        }
        if(t < 2.5f / d1) {
            t -= 2.25f / d1;
            return n1 * t * t + 0.9375f;
        }
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }

    //! 正弦波でなめらかに進む値を返します。
    //! @param  [in] t 進み具合（0〜1）
    //! @return 変換した値
    [[nodiscard]] inline float InOutSine(float t) {
        t = Clamp01(t);
        return 0.5f - 0.5f * std::cos(PI * t);
    }

    //! 0 から行き過ぎて揺れながら 0 に戻る値を返します。
    //! @param  [in] t 進み具合（0〜1）
    //! @return 変換した値（最初の山で 1 近く、両端は 0）
    [[nodiscard]] inline float Punch(float t) {
        t = Clamp01(t);
        if(t >= 1.0f)
            return 0.0f;
        // 2回半揺れながら、だんだん小さくする
        return std::sin(t * PI * 2.5f) * (1.0f - t);
    }

    //! 種類を指定してイージングを計算します。
    //! @param  [in] type 種類
    //! @param  [in] t    進み具合（0〜1。範囲外は収める）
    //! @return 変換した値
    [[nodiscard]] inline float Evaluate(EaseType type, float t) {
        switch(type) {
        case EaseType::InQuad:
            return InQuad(t);
        case EaseType::OutQuad:
            return OutQuad(t);
        case EaseType::InOutQuad:
            return InOutQuad(t);
        case EaseType::InCubic:
            return InCubic(t);
        case EaseType::OutCubic:
            return OutCubic(t);
        case EaseType::InOutCubic:
            return InOutCubic(t);
        case EaseType::OutBack:
            return OutBack(t);
        case EaseType::OutElastic:
            return OutElastic(t);
        case EaseType::OutBounce:
            return OutBounce(t);
        case EaseType::InOutSine:
            return InOutSine(t);
        case EaseType::Punch:
            return Punch(t);
        case EaseType::Linear:
        default:
            return Linear(t);
        }
    }

    //! 目標の値へ、フレームレートに依存せずなめらかに近づけた値を返します。
    //! @param  [in] current   今の値
    //! @param  [in] target    目標の値
    //! @param  [in] speed     近づく速さ（1秒あたりの減衰率。大きいほど速い。0 以下なら動かない）
    //! @param  [in] deltaTime 前フレームからの経過時間（秒）
    //! @return 近づけた値
    [[nodiscard]] inline float Damp(float current, float target, float speed, float deltaTime) {
        if(speed <= 0.0f || deltaTime <= 0.0f)
            return current;
        return target + (current - target) * std::exp(-speed * deltaTime);
    }

    //! 目標の値へ、フレームレートに依存せずなめらかに近づけた値を返します（倍精度）。
    //! @param  [in] current   今の値
    //! @param  [in] target    目標の値
    //! @param  [in] speed     近づく速さ（1秒あたりの減衰率。大きいほど速い。0 以下なら動かない）
    //! @param  [in] deltaTime 前フレームからの経過時間（秒）
    //! @return 近づけた値
    [[nodiscard]] inline double Damp(double current, double target, float speed, float deltaTime) {
        if(speed <= 0.0f || deltaTime <= 0.0f)
            return current;
        return target + (current - target) * std::exp(-static_cast<double>(speed) * static_cast<double>(deltaTime));
    }
}    // namespace Tsukino::Core::Math::Easing

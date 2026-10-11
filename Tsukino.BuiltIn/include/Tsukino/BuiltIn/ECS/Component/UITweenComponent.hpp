//----------------------------------------------------------------------------
//! @file   UITweenComponent.hpp
//! @brief  画面UIのトゥイーン（決めた時間で値をイージングしながら動かす）のコンポーネント
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Easing.hpp>

#include <cstdint>
#include <vector>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! トゥイーンで動かす値です。
    enum class UITweenProperty {
        OffsetX,     // UIMotionComponent::offset.x（足し合わせる）
        OffsetY,     // UIMotionComponent::offset.y（足し合わせる）
        Scale,       // UIMotionComponent::scale の x・y 両方（掛け合わせる）
        ScaleX,      // UIMotionComponent::scale.x（掛け合わせる）
        ScaleY,      // UIMotionComponent::scale.y（掛け合わせる）
        Rotation,    // UIMotionComponent::rotation（ラジアン。足し合わせる）
        Alpha,       // UICanvasGroupComponent::alpha（掛け合わせる）
    };

    //! トゥイーンの繰り返し方です。
    enum class UITweenLoop {
        Once,        // 1回だけ。終わったら取り除く
        Loop,        // from → to を繰り返す
        PingPong,    // from → to → from を繰り返す
    };

    //! トゥイーン1本です。
    //! @note   値は from + (to - from) × ease(進み具合)。ease が Punch のときは終わりで from に戻る。
    //!         delay の間は from の値を当てる（出る前に一瞬見えてしまわないように）。
    //!         holdAtEnd が true の Once は、終わっても取り除かずに to を当て続ける
    //!         （フェードアウトの後に一瞬元に戻って見えないように。Stop か同じ tag の再生で外す）
    struct UITween {
        UITweenProperty                       property    = UITweenProperty::Scale;    // 動かす値
        float                                 from        = 1.0f;                      // 始めの値
        float                                 to          = 1.0f;                      // 終わりの値
        float                                 duration    = 0.25f;                     // 1回の長さ（秒）
        float                                 delay       = 0.0f;                      // 始めるまでの待ち（秒）
        float                                 repeatDelay = 0.0f;                      // 繰り返しの間の待ち（秒。Loop・PingPong のとき）
        Tsukino::Core::Math::Easing::EaseType ease        = Tsukino::Core::Math::Easing::EaseType::OutCubic;    // 緩急の付け方
        UITweenLoop                           loop        = UITweenLoop::Once;                                  // 繰り返し方
        bool                                  holdAtEnd   = false;                                              // Once が終わっても終わりの値を当て続けるか
        bool                                  unscaledTime = false;    // ゲームの時間の止まり・スローに左右されずに進めるか（UnscaledTimeContext。メニューの UI など）
        std::uint32_t                         tag         = 0;      // 種類の目印（同じ tag を再生し直すと置き換わる）
        float                                 elapsed     = 0.0f;    // 経過時間（秒。delay を含む。UITweenSystem が進める）
    };

    //! 画面UIのトゥイーンです。
    //! @note   UITweenSystem が毎フレーム時間を進め、UIMotionComponent と UICanvasGroupComponent を
    //!         作り直す（無ければ付ける）。同じ値を動かすトゥイーンが複数あれば合成する
    //!         （拡大率と α は掛け合わせ、ずれと回転は足し合わせる）。
    //!         再生・停止は UITweenUtility の Play / Stop を使う
    struct UITweenComponent {
        std::vector<UITween> tweens;    // 再生中のトゥイーン
    };
}    // namespace Tsukino::BuiltIn::ECS

//----------------------------------------------------------------------------
//! @file   UnscaledTimeContext.hpp
//! @brief  ゲームの時間の止まり・スローに左右されない経過時間のコンテキスト
//! @detail ゲームはポーズ中やメニュー中にシステムへ deltaTime = 0 を渡したり、スロー演出で
//!         deltaTime を縮めたりします。メニューの UI はその間も動かしたいので、実時間の経過を
//!         レジストリのコンテキストとして別に渡します。シーンが毎フレーム設定し、
//!         UITweenSystem が UITween::unscaledTime のトゥイーンとボタンの手ごたえを進めるのに使います。
//----------------------------------------------------------------------------
#pragma once

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! ゲームの時間の止まり・スローに左右されない経過時間です。
    //! @note   registry.SetContext<UnscaledTimeContext>(realDeltaTime) のように毎フレーム設定します。
    //!         設定されていなければ、受け取る側はシステムへ渡された deltaTime をそのまま使います
    struct UnscaledTimeContext {
        float deltaTime = 0.0f;    // 前フレームからの実時間の経過（秒）
    };

}    // namespace Tsukino::BuiltIn::ECS

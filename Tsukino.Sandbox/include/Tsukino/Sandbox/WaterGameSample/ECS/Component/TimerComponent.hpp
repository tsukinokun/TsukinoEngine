//-------------------------------------------------------------
//! @file   TimerComponent.hpp
//! @brief  カウントダウンタイマー用コンポーネント
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    //-------------------------------------------------------------
    //! @struct TimerComponent
    //! @brief  durationから0へ向かって減っていくカウントダウンタイマー
    //-------------------------------------------------------------
    struct TimerComponent {
        float duration      = 60.0f;    //!< 制限時間の初期値（秒）
        float remainingTime = 60.0f;    //!< 現在の残り時間（秒）。初期化時はdurationと同じ値にしておく
        bool  isRunning     = true;     //!< false中は時間が進まない（ポーズ用）
        bool  isFinished    = false;    //!< 0に達したか（TimerSystemが設定。1回だけtrueになる）
        bool  hasNotified   = false;    //!< TimeUpEventを既に発行したか（重複発行防止。TimerSystem内部で管理）
    };

}    // namespace WaterGame::ECS

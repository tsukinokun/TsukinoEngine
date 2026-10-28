//----------------------------------------------------------------------------
//! @file   LifetimeComponent.hpp
//! @brief  時間が来たら子ごと片付ける印のコンポーネント
//----------------------------------------------------------------------------
#pragma once

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 時間が来たら子ごと片付ける印です。
    //! @note   通知・得点のポップ・滑り出して消える UI など、しばらく見せたら消すものに付ける。
    //!         LifetimeSystem が残り時間を減らし、0 以下になったらそのエンティティと
    //!         Transform の子孫をまとめて破棄する（TransformUtility::DestroyHierarchy）
    struct LifetimeComponent {
        float remaining = 0.0f;    // 残り時間（秒）
    };
}    // namespace Tsukino::BuiltIn::ECS

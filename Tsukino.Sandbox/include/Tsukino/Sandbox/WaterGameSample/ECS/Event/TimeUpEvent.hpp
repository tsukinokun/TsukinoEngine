//-------------------------------------------------------------
//! @file   TimeUpEvent.hpp
//! @brief  カウントダウンタイマーが0になった際のイベント
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

#include <entt/entt.hpp>

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    //-------------------------------------------------------------
    //! @struct TimeUpEvent
    //! @brief  カウントダウンタイマーが0秒に達した際に発行されるイベント
    //-------------------------------------------------------------
    struct TimeUpEvent {
        entt::entity timerEntity;    //!< 時間切れになったTimerComponentを持つエンティティ
    };

}    // namespace WaterGame::ECS

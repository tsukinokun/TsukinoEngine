//-------------------------------------------------------------
//! @file   DotEatenEvent.hpp
//! @brief  ドットが食べられた際のイベント
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

#include <entt/entt.hpp>

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    //-------------------------------------------------------------
    //! @struct DotEatenEvent
    //! @brief  SEやパーティクル等の副作用処理に使う通知イベント
    //-------------------------------------------------------------
    struct DotEatenEvent {
        entt::entity eater;         //!< 食べた側のエンティティ（ボール）
        entt::entity dot;           //!< 食べられたドットのエンティティ
        int          scoreValue;    //!< 加算されたスコア値
    };

}    // namespace WaterGame::ECS

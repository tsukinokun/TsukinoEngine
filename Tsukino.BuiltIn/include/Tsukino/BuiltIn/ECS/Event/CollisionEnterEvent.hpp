//-------------------------------------------------------------
//! @file   CollisionEnterEvent.hpp
//! @brief  衝突開始イベント
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>

#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct CollisionEnterEvent
    //! @brief  2つのエンティティが衝突した際に発行されるイベント
    //! @note   PhysicsSystemがPublishし、各SystemがSubscribeして使う
    //-------------------------------------------------------------
    struct CollisionEnterEvent {
        Tsukino::ECS::Entity self;      //!< 衝突したエンティティ
        Tsukino::ECS::Entity other;     //!< 衝突相手のエンティティ
        hlslpp::float3       normal;    //!< 衝突時の法線（selfから見たotherへの向き）
    };
}    // namespace Tsukino::BuiltIn::ECS

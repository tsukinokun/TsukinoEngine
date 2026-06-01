//-------------------------------------------------------------
//! @file   EntityEvent.hpp
//! @brief  エンティティのライフサイクルに関するBuilt-inイベント
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
// 名前空間 : Tsukino::ECS::EngineEvent
// エンジンのシーン管理に関するイベントを定義する名前空間、特別にエンジン層に置く
namespace Tsukino::ECS::EngineEvent {

    //-------------------------------------------------------------
    //! @struct EntityCreatedEvent
    //! @brief  エンティティが生成されたとき
    //! @note   Scene::CreateEntity() が自動で Publish する
    //-------------------------------------------------------------
    struct EntityCreatedEvent {
        Entity entity;    //!< 生成されたエンティティのID
    };

    //-------------------------------------------------------------
    //! @struct EntityDestroyedEvent
    //! @brief  エンティティが破棄される直前
    //! @note   Scene::DestroyEntity() が破棄前に自動で Publish する
    //!         破棄後に Publish するとEntityが無効になるため必ず破棄前
    //-------------------------------------------------------------
    struct EntityDestroyedEvent {
        Entity entity;    //!< 破棄されるエンティティのID
    };

}    // namespace Tsukino::ECS::EngineEvent

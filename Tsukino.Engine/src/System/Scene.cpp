//-------------------------------------------------------------
//! @file   Scene.cpp
//! @brief  シーンクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Engine/ECS/Scene.hpp>
#include <Tsukino/Engine/ECS/EngineEvent/EntityEvent.hpp>
#include <Tsukino/Engine/ECS/EngineEvent/SceneEvent.hpp>

// 名前空間 : Tsukino::ECS
namespace Tsukino::ECS {
    //-------------------------------------------------------------
    //! @brief  シーン初期化
    //-------------------------------------------------------------
    void Scene::Initialize() {
        //-------------------------------------------------------------
        // EventBus を Registry のコンテキストに登録する
        // これにより System 内で
        // registry.GetContext<EventBus*>() からアクセスできる
        //-------------------------------------------------------------
        m_registry.SetContext<EventBus*>(&m_eventBus);

        //-------------------------------------------------------------
        // シーン初期化完了を Built-in イベントで通知する
        //-------------------------------------------------------------
        m_eventBus.Publish(EngineEvent::SceneInitializedEvent{});
    }

    //-------------------------------------------------------------
    //! @brief  毎フレームの更新
    //-------------------------------------------------------------
    void Scene::Update(float deltaTime) {
        // 登録されている全てのシステムを実行
        m_systemManager.Update(m_registry, deltaTime);
    }

    //-------------------------------------------------------------
    //! @brief  エンティティの生成
    //-------------------------------------------------------------
    Entity Scene::CreateEntity() {
        Tsukino::ECS::Entity entity = m_registry.CreateEntity();

        //-------------------------------------------------------------
        // エンティティ生成を Built-in イベントで通知する
        //-------------------------------------------------------------
        m_eventBus.Publish(EngineEvent::EntityCreatedEvent{entity});

        return entity;
    }

    //-------------------------------------------------------------
    //! @brief  エンティティの破棄
    //-------------------------------------------------------------
    void Scene::DestroyEntity(Entity entity) {
        //-------------------------------------------------------------
        // 破棄後は Entity が無効になるため、破棄前に通知する
        //-------------------------------------------------------------
        m_eventBus.Publish(EngineEvent::EntityDestroyedEvent{entity});
        m_registry.DestroyEntity(entity);
    }

    //-------------------------------------------------------------
    //! @brief  システムの追加
    //-------------------------------------------------------------
    void Scene::AddSystem(std::shared_ptr<ISystem> system, int priority) {
        m_systemManager.AddSystem(std::move(system), priority);
    }

}    // namespace Tsukino::ECS

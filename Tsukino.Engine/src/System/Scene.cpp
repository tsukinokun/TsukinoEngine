//-------------------------------------------------------------
//! @file   Scene.cpp
//! @brief  シーンクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Engine/ECS/Scene.hpp>

// 名前空間 : Tsukino::ECS
namespace Tsukino::ECS {
    //-------------------------------------------------------------
    //! @brief  シーン初期化
    //-------------------------------------------------------------
    void Scene::Initialize() {
        // 今後の拡張として、Sceneロード時や初期化時に必要な処理を記述
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
        return m_registry.CreateEntity();
    }

    //-------------------------------------------------------------
    //! @brief  エンティティの破棄
    //-------------------------------------------------------------
    void Scene::DestroyEntity(Entity entity) {
        m_registry.DestroyEntity(entity);
    }

    //-------------------------------------------------------------
    //! @brief  システムの追加
    //-------------------------------------------------------------
    void Scene::AddSystem(std::shared_ptr<ISystem> system, int priority) {
        m_systemManager.AddSystem(std::move(system), priority);
    }

}    // namespace Tsukino::ECS

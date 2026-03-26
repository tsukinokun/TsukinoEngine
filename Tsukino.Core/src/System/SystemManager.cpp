//-------------------------------------------------------------
//! @file   SystemManager.cpp
//! @brief  システムマネージャーの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Core/ECS/System/SystemManager.hpp>
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <algorithm>
// 名前空間 : Tsukino::ECS
namespace Tsukino::ECS {
    //-------------------------------------------------------------
    //! @brief  システムの追加
    //-------------------------------------------------------------
    void SystemManager::AddSystem(std::shared_ptr<ISystem> system, int priority) {
        m_systems.push_back({std::move(system), priority});

        // 追加時に優先度で昇順ソート（priority が小さい要素が前になる）
        std::sort(m_systems.begin(), m_systems.end(), [](const SystemEntry& a, const SystemEntry& b) { return a.priority < b.priority; });
    }

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void SystemManager::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        for(auto& entry : m_systems) {
            if(entry.system) {
                entry.system->Update(registry, deltaTime);
            }
        }
    }

    //-------------------------------------------------------------
    //! @brief  システムのクリア
    //-------------------------------------------------------------
    void SystemManager::Clear() {
        m_systems.clear();
    }

}    // namespace Tsukino::ECS

//-------------------------------------------------------------
//! @file   InteractionSystem.cpp
//! @brief  InteractionSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/InteractionSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <hlsl++.h>
#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void InteractionSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        // 一旦仮実装
    }

}    // namespace Tsukino::BuiltIn::ECS

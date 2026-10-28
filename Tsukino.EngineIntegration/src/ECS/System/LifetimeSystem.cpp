//----------------------------------------------------------------------------
//! @file   LifetimeSystem.cpp
//! @brief  時間が来たエンティティを子ごと片付けるシステムの実装
//----------------------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/LifetimeSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/LifetimeComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Transform/TransformUtility.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <vector>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //----------------------------------------------------------------------------
    //! 残り時間を減らして片付けます。
    //----------------------------------------------------------------------------
    void LifetimeSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        // 子孫を探すには Transform を総なめするので、時間が来たものを先に集めてから破棄する
        std::vector<Tsukino::ECS::Entity> expired;
        registry.View<LifetimeComponent>().each([&](Tsukino::ECS::Entity entity, LifetimeComponent& lifetime) {
            lifetime.remaining -= deltaTime;
            if(lifetime.remaining <= 0.0f) {
                expired.push_back(entity);
            }
        });
        for(const Tsukino::ECS::Entity entity : expired) {
            registry.RemoveComponent<LifetimeComponent>(entity);
            TransformUtility::DestroyHierarchy(registry, entity);
        }
    }
}    // namespace Tsukino::BuiltIn::ECS

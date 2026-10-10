//--------------------------------------------------------------
//! @file   UIVisibilityUtility.hpp
//! @brief  画面UIの表示・非表示（UIVisibilityComponent）を引くヘルパー
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIVisibilityComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Transform/TransformUtility.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <entt/entt.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS::UIVisibilityUtility
namespace Tsukino::BuiltIn::ECS::UIVisibilityUtility {

    //--------------------------------------------------------------
    //! @brief  エンティティが隠れているか（自身か祖先の UIVisibilityComponent::visible が false）を返す
    //! @param  registry [in] ECS レジストリ
    //! @param  entity   [in] 調べるエンティティ
    //! @return 隠れていれば true（UIVisibilityComponent がどこにも無ければ false）
    //! @note   祖先の辿り方は TransformUtility::FindNearestWith と同じ（深すぎる・循環しているときは隠れていない扱い）
    //--------------------------------------------------------------
    [[nodiscard]] inline bool IsHidden(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
        Tsukino::ECS::Entity current = entity;
        for(int depth = 0; depth < TransformUtility::kMaxHierarchyDepth; ++depth) {
            if(current == entt::null)
                return false;

            if(const auto* visibility = registry.try_get<UIVisibilityComponent>(current); visibility && !visibility->visible)
                return true;

            const auto* transform = registry.try_get<TransformComponent>(current);
            if(!transform)
                return false;
            current = transform->parent;
        }
        return false;
    }
}    // namespace Tsukino::BuiltIn::ECS::UIVisibilityUtility

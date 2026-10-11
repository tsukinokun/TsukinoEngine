//----------------------------------------------------------------------------
//! @file   UICanvasGroupUtility.hpp
//! @brief  画面UIのまとめての透明度（UICanvasGroupComponent）を引くヘルパー
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UICanvasGroupComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Transform/TransformUtility.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <entt/entt.hpp>

#include <algorithm>

// 名前空間 : Tsukino::BuiltIn::ECS::UICanvasGroupUtility
namespace Tsukino::BuiltIn::ECS::UICanvasGroupUtility {

    //! これより小さい不透明度は「見えない」とみなします（当たり判定を外すしきい値）。
    inline constexpr float kInvisibleAlpha = 0.01f;

    //! 自身と祖先の UICanvasGroupComponent::alpha を掛け合わせた不透明度を返します。
    //! @param  [in] registry ECS レジストリ
    //! @param  [in] entity   調べるエンティティ
    //! @return 不透明度（0〜1。UICanvasGroupComponent がどこにも無ければ 1）
    //! @note   祖先の辿り方は UIVisibilityUtility::IsHidden と同じ（深すぎる・循環しているときはそこで打ち切る）
    [[nodiscard]] inline float GetAlpha(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
        float                alpha   = 1.0f;
        Tsukino::ECS::Entity current = entity;
        for(int depth = 0; depth < TransformUtility::kMaxHierarchyDepth; ++depth) {
            if(current == entt::null || !registry.IsValid(current))
                break;

            if(const auto* group = registry.try_get<UICanvasGroupComponent>(current))
                alpha *= std::clamp(group->alpha, 0.0f, 1.0f);

            const auto* transform = registry.try_get<TransformComponent>(current);
            if(!transform)
                break;
            current = transform->parent;
        }
        return alpha;
    }

    //! 自身か祖先の UICanvasGroupComponent でほぼ透明になっているかを返します。
    //! @param  [in] registry ECS レジストリ
    //! @param  [in] entity   調べるエンティティ
    //! @return ほぼ透明なら true
    [[nodiscard]] inline bool IsInvisible(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
        return GetAlpha(registry, entity) < kInvisibleAlpha;
    }
}    // namespace Tsukino::BuiltIn::ECS::UICanvasGroupUtility

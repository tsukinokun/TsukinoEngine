//----------------------------------------------------------------------------
//! @file   UIToggleUtility.hpp
//! @brief  画面UIの切り替えの組を、コードから読み書きするヘルパー
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/UIToggleComponent.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS::UIToggleUtility
namespace Tsukino::BuiltIn::ECS::UIToggleUtility {

    //! 組の中で入になっている切り替えを返します。
    //! @param  [in] registry ECS レジストリ
    //! @param  [in] group    組（UIToggleGroupComponent を持つエンティティ）
    //! @return 入の切り替え（無ければ entt::null）
    [[nodiscard]] inline Tsukino::ECS::Entity GetSelected(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity group) {
        Tsukino::ECS::Entity selected = entt::null;
        registry.View<UIToggleComponent>().each([&](Tsukino::ECS::Entity entity, const UIToggleComponent& toggle) {
            if(selected == entt::null && toggle.isOn && static_cast<Tsukino::ECS::Entity>(toggle.group) == group) {
                selected = entity;
            }
        });
        return selected;
    }

    //! 切り替えを入にし、同じ組の他を切にします（changed は立てない。初期値を合わせるとき用）。
    //! @param  [in] registry ECS レジストリ
    //! @param  [in] entity   入にする切り替え
    inline void Select(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
        auto* chosen = registry.try_get<UIToggleComponent>(entity);
        if(chosen == nullptr) {
            return;
        }
        const Tsukino::ECS::Entity group = chosen->group;
        if(group != entt::null) {
            registry.View<UIToggleComponent>().each([&](UIToggleComponent& other) {
                if(static_cast<Tsukino::ECS::Entity>(other.group) == group) {
                    other.isOn = false;
                }
            });
        }
        chosen->isOn = true;
    }
}    // namespace Tsukino::BuiltIn::ECS::UIToggleUtility

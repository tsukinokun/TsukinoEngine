//----------------------------------------------------------------------------
//! @file   UIToggleSystem.cpp
//! @brief  画面UIの切り替えをクリックで入・切するシステムの実装
//----------------------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/UIToggleSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/PointerTargetComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UIToggleComponent.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <vector>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //----------------------------------------------------------------------------
    //! 切り替えを更新します。
    //----------------------------------------------------------------------------
    void UIToggleSystem::Update(Tsukino::ECS::Registry& registry, float /*deltaTime*/) {
        registry.View<UIToggleGroupComponent>().each([](UIToggleGroupComponent& group) { group.changed = false; });

        // 押されたものを先に集める（組の他の切り替えを書き換えながら同じ View を回さないように）
        std::vector<Tsukino::ECS::Entity> pressed;
        registry.View<UIToggleComponent, PointerTargetComponent>().each(
            [&](Tsukino::ECS::Entity entity, UIToggleComponent& toggle, const PointerTargetComponent& target) {
                toggle.changed = false;
                if(target.clicked && toggle.interactable) {
                    pressed.push_back(entity);
                }
            });

        for(const Tsukino::ECS::Entity entity : pressed) {
            UIToggleComponent&         toggle = registry.GetComponent<UIToggleComponent>(entity);
            const Tsukino::ECS::Entity group  = toggle.group;
            auto*                      marker = (group != entt::null) ? registry.try_get<UIToggleGroupComponent>(group) : nullptr;
            if(marker == nullptr) {
                toggle.isOn    = !toggle.isOn;
                toggle.changed = true;
                continue;
            }
            if(toggle.isOn) {
                continue;    // 組の中で選ばれているものを押しても、何も変わらない
            }
            registry.View<UIToggleComponent>().each([&](UIToggleComponent& other) {
                if(static_cast<Tsukino::ECS::Entity>(other.group) == group && other.isOn) {
                    other.isOn    = false;
                    other.changed = true;
                }
            });
            toggle.isOn     = true;
            toggle.changed  = true;
            marker->changed = true;
        }
    }
}    // namespace Tsukino::BuiltIn::ECS

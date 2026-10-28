//----------------------------------------------------------------------------
//! @file   UISliderSystem.cpp
//! @brief  画面UIのスライダーをマウスで動かすシステムの実装
//----------------------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/UISliderSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/PointerTargetComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/UISliderComponent.hpp>
#include <Tsukino/BuiltIn/ECS/UI/UICanvas.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <algorithm>
#include <cmath>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //----------------------------------------------------------------------------
    //! スライダーの値を更新します。
    //----------------------------------------------------------------------------
    void UISliderSystem::Update(Tsukino::ECS::Registry& registry, float /*deltaTime*/) {
        Tsukino::EngineIntegration::EngineContext* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!context || !context->inputSystem) {
            return;
        }
        auto view = registry.View<UISliderComponent, PointerTargetComponent>();
        if(view.begin() == view.end()) {
            return;    // 使っていないシーンでは、マウスの変換も省く
        }

        const Tsukino::Input::InputSystem& input = *context->inputSystem;
        i32                                mouseX = 0;
        i32                                mouseY = 0;
        input.GetMousePosition(&mouseX, &mouseY);
        const hlslpp::float2 mouse      = GetUICanvas(registry).ToUI(hlslpp::float2(static_cast<float>(mouseX), static_cast<float>(mouseY)));
        const bool           buttonDown = input.IsKeyDown(Tsukino::Input::KeyCode::LButton);

        view.each([&](Tsukino::ECS::Entity entity, UISliderComponent& slider, const PointerTargetComponent& target) {
            slider.dragStarted = false;
            slider.dragEnded   = false;
            slider.changed     = false;

            if(!slider.interactable) {
                slider.dragEnded = slider.dragging;
                slider.dragging  = false;
                return;
            }
            if(target.clicked && !slider.dragging) {
                slider.dragging    = true;
                slider.dragStarted = true;
            }
            if(!slider.dragging) {
                return;
            }

            //----------------------------------------------------------------------------
            // マウスを線のローカル座標へ戻し、X を長さで割って値にする。
            // 線が傾いて揺れていても、その向きに沿って測れる
            //----------------------------------------------------------------------------
            const Tsukino::ECS::Entity track       = slider.track;
            const Tsukino::ECS::Entity trackEntity = (track != entt::null) ? track : entity;
            if(const auto* transform = registry.try_get<TransformComponent>(trackEntity); transform != nullptr && slider.trackLength > 0.0f) {
                const hlslpp::float4 local = hlslpp::mul(hlslpp::float4(mouse, 0.0f, 1.0f), hlslpp::inverse(transform->worldMatrix));
                const float          t     = std::clamp(float(local.x) / slider.trackLength, 0.0f, 1.0f);
                const float          value = slider.minValue + (slider.maxValue - slider.minValue) * t;
                if(std::abs(value - slider.value) > slider.changeThreshold) {
                    slider.value   = value;
                    slider.changed = true;
                }
            }

            if(!buttonDown) {
                slider.dragging  = false;
                slider.dragEnded = true;
            }
        });
    }
}    // namespace Tsukino::BuiltIn::ECS

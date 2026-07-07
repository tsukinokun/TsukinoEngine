//-------------------------------------------------------------
//! @file   TimerSystem.cpp
//! @brief  TimerSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/WaterGameSample/ECS/System/TimerSystem.hpp>

#include <Tsukino/Sandbox/WaterGameSample/ECS/Component/TimerComponent.hpp>
#include <Tsukino/Sandbox/WaterGameSample/ECS/Component/TimeUIComponent.hpp>
#include <Tsukino/Sandbox/WaterGameSample/ECS/Event/TimeUpEvent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>

#include <Tsukino/Core/ECS/Event/EventBus.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <Tsukino/Core/ECS/Event/EventBus.hpp>

#include <algorithm>

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    //------------------------------------------------------------
    //! @brief  コンストラクタ
    //------------------------------------------------------------
    TimerSystem::TimerSystem(Tsukino::ECS::EventBus& eventBus)
        : m_eventBus(eventBus) {
    }

    //-------------------------------------------------------------
    //! @brief  システムの更新処理
    //-------------------------------------------------------------
    void TimerSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto view = registry.View<TimerComponent>();

        view.each([&](auto entity, auto& timer) {
            if(!timer.isRunning || timer.isFinished)
                return;

            timer.remainingTime -= deltaTime;

            auto view = registry.View<TimeUIComponent>();

            view.each([&](auto uiEntity, auto& timeUI) {
                Tsukino::BuiltIn::ECS::FontComponent& font = registry.GetComponent<Tsukino::BuiltIn::ECS::FontComponent>(uiEntity);
                font.text                                  = L"Time: " + std::to_wstring(static_cast<int>(timer.remainingTime));
            });

            if(timer.remainingTime <= 0.0f) {
                timer.remainingTime = 0.0f;

                timer.isFinished = true;

                if(!timer.hasNotified) {
                    timer.hasNotified = true;
                    m_eventBus.Publish(TimeUpEvent{entity});
                }
            }
        });
    }

}    // namespace WaterGame::ECS

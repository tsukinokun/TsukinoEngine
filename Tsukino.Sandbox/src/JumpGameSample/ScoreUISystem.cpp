//-------------------------------------------------------------
//! @file   ScoreUISystem.cpp
//! @brief  ScoreUISystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/JumpGameSample/ECS/System/ScoreUISystem.hpp>

#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/ScoreUIComponent.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/ScoreComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>

#include <hlsl++.h>
// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void ScoreUISystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // コンテキストの取得
        //-------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx)
            return;

        //-------------------------------------------------------------
        // viewを取得してScoreUIを更新
        //-------------------------------------------------------------
        auto uiView = registry.View<Tsukino::BuiltIn::ECS::FontComponent, JumpGameSample::ECS::ScoreUIComponent>();
        uiView.each([&](Tsukino::ECS::Entity entity, Tsukino::BuiltIn::ECS::FontComponent& transform, JumpGameSample::ECS::ScoreUIComponent& Scoreui) {
            auto scoreView = registry.View<JumpGameSample::ECS::ScoreComponent>();
            scoreView.each([&](Tsukino::ECS::Entity entity, JumpGameSample::ECS::ScoreComponent& score) {
                transform.text = L"Score: " + std::to_wstring(static_cast<int>(score.value));    // 描画するテキスト
            });
        });
    }
}    // namespace JumpGameSample::ECS

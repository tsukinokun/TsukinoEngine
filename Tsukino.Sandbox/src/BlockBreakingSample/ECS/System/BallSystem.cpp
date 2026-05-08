//-------------------------------------------------------------
//! @file   BallSystem.cpp
//! @brief  BallSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/System/BallSystem.hpp>
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/Component/BallComponent.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <hlsl++.h>
// 名前空間 : BlockBreakingSample::ECS
namespace BlockBreakingSample::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void BallSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // コンテキストの取得
        //-------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx)
            return;

        //-------------------------------------------------------------
        // viewを取得して各パドルを更新
        //-------------------------------------------------------------
        auto view = registry.View<Tsukino::BuiltIn::ECS::TransformComponent, BallComponent>();
        view.each([&](entt::entity entity, Tsukino::BuiltIn::ECS::TransformComponent& transform, BallComponent& paddle) {});
    }
}    // namespace BlockBreakingSample::ECS

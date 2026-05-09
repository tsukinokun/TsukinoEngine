//-------------------------------------------------------------
//! @file   BrickSystem.cpp
//! @brief  BrickSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/System/BrickSystem.hpp>
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/Component/BrickComponent.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <hlsl++.h>
// 名前空間 : BlockBreakingSample::ECS
namespace BlockBreakingSample::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void BrickSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // コンテキストの取得
        //-------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx)
            return;

        Tsukino::Input::InputSystem* inputSystem = ctx->inputSystem;

        //-------------------------------------------------------------
        // viewを取得して各パドルを更新
        //-------------------------------------------------------------
        auto view = registry.View<Tsukino::BuiltIn::ECS::TransformComponent, BrickComponent>();
        view.each([&](entt::entity entity, Tsukino::BuiltIn::ECS::TransformComponent& transform, BrickComponent& brick) {
            if(brick.dead) {
                // ブロックが壊れている場合はエンティティを破棄
                registry.DestroyEntity(entity);
            }
        });
    }
}    // namespace BlockBreakingSample::ECS

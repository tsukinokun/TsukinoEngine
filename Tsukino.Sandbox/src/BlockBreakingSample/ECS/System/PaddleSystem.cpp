//-------------------------------------------------------------
//! @file   PaddleSystem.cpp
//! @brief  PaddleSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/System/PaddleSystem.hpp>
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/Component/PaddleComponent.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <hlsl++.h>
// 名前空間 : BlockBreakingSample::ECS
namespace BlockBreakingSample::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void PaddleSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
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
        auto view = registry.View<Tsukino::BuiltIn::ECS::TransformComponent, PaddleComponent>();
        view.each([&](entt::entity entity, Tsukino::BuiltIn::ECS::TransformComponent& transform, PaddleComponent& paddle) {
            //-------------------------------------------------------------
            // パドルの移動処理
            //-------------------------------------------------------------
            if(inputSystem->IsKeyDown(Tsukino::Input::KeyCode::Left)) {
                // 左キーが押されている場合の処理
                transform.position.x -= paddle.speed * deltaTime;
                transform.dirty       = true;    // Transformが更新されたことを示すフラグ
            } else if(inputSystem->IsKeyDown(Tsukino::Input::KeyCode::Right)) {
                // 右キーが押されている場合の処理
                transform.position.x += paddle.speed * deltaTime;
                transform.dirty       = true;    // Transformが更新されたことを示すフラグ
            }
        });
    }
}    // namespace BlockBreakingSample::ECS

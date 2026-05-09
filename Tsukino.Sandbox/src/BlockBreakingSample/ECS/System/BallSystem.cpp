//-------------------------------------------------------------
//! @file   BallSystem.cpp
//! @brief  BallSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/System/BallSystem.hpp>
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/Component/BallComponent.hpp>
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/Component/PaddleComponent.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>

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
        auto* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx)
            return;

        // パドルの情報を取得しておく
        auto paddleView = registry.View<Tsukino::BuiltIn::ECS::TransformComponent, PaddleComponent>();

        // 最初のパドルを取得（通常は1つ）
        auto paddleEntity = paddleView.front();

        if(paddleEntity == entt::null)
            return;    // パドルがなければ何もしない

        auto& paddleTransform = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(paddleEntity);

        // ボールの更新ループ
        auto ballView = registry.View<Tsukino::BuiltIn::ECS::TransformComponent, BallComponent>();
        ballView.each([&](entt::entity entity, Tsukino::BuiltIn::ECS::TransformComponent& ballTrans, BallComponent& ball) {
            auto* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();

            Tsukino::BuiltIn::ECS::RigidbodyComponent* rbPtr = nullptr;
            if(registry.HasComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(entity))
                rbPtr = &registry.GetComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(entity);

            // -------------------------
            // 発射前：パドルに追従
            // -------------------------
            if(!ball.IsLaunched) {
                ballTrans.position = paddleTransform.position + ball.offset;
                ballTrans.dirty    = true;

                // 発射
                if(ctx->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Space) && rbPtr) {
                    ball.IsLaunched = true;

                    // 奥方向は +Y
                    hlslpp::float3 dir    = hlslpp::normalize(hlslpp::float3(0.1f, 1.0f, 0.0f));
                    rbPtr->linearVelocity = dir * ball.speed;
                }

                return;
            }

            // -------------------------
            // 発射後：Kinematic 移動
            // -------------------------
            if(rbPtr) {
                ballTrans.position += rbPtr->linearVelocity * deltaTime;
                ballTrans.dirty     = true;
            }
        });
    }
}    // namespace BlockBreakingSample::ECS

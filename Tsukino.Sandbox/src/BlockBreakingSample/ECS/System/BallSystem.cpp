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
            if(!ball.IsLaunched) {
                // --- 発射前：パドルに追従 ---
                // パドルの位置 + 設定されたオフセットをボールの座標にする
                ballTrans.position = paddleTransform.position + ball.offset;
                ballTrans.dirty    = true;

                // もし CollisionComponent (Jolt) があるなら、物理ワールド側の座標もワープさせる
                if(registry.HasComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(entity)) {
                    auto& coll = registry.GetComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(entity);
                    // 本来は BodyInterface を経由するが、Transformの同期システムが別途あれば不要
                    // ctx->physicsWorld->GetBodyInterface().SetPosition(...) など
                }

                // --- 発射判定：スペースキーなどで発射 ---
                if(ctx->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Space)) {
                    ball.IsLaunched = true;

                    // 物理的に打ち出す処理（物理エンジンへの速度設定）が必要
                    // 例:
                    // auto& rb = registry.GetComponent<RigidbodyComponent>(entity);
                    // rb.velocity = hlslpp::float3(0, 0, ball.speed);
                }
            } else {
                // --- 発射後：必要なら速度の維持などのロジックをここに書く ---
            }
        });
    }
}    // namespace BlockBreakingSample::ECS

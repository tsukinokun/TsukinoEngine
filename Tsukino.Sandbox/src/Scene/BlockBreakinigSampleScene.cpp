//-------------------------------------------------------------
//! @file    BlockBreakingSampleScene.cpp
//! @brief   サンプルシーン1の実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/Scene/BlockBreakingSampleScene.hpp>

#include <Tsukino/Sandbox/BlockBreakingSample/ECS/Component/PaddleComponent.hpp>
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/Component/BallComponent.hpp>
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/System/PaddleSystem.hpp>
#include <Tsukino/Sandbox/BlockBreakingSample/ECS/System/BallSystem.hpp>

#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Core/Path.hpp>

// 必要なシステムとコンポーネントのインクルード
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/AudioSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidBodyComponent.hpp>
#include <Tsukino/EngineIntegration/ECS/System/PhysicsSystem.hpp>

#include <entt/entt.hpp>
#include <hlsl++.h>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    //-------------------------------------------------------------
    //! @brief  シーン固有の初期化処理
    //-------------------------------------------------------------
    void BlockBreakingSampleScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) {
        // コンテキストをレジストリから取得
        Tsukino::EngineIntegration::EngineContext* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();

        // Transformは一番最初に計算する (優先度 0)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), 0);
        // カメラは描画前に更新する (優先度 5)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), 5);
        // パドルの操作 (優先度 6)
        m_scene.AddSystem(std::make_shared<BlockBreakingSample::ECS::PaddleSystem>(), 6);
        // ボールの操作 (優先度 7)
        m_scene.AddSystem(std::make_shared<BlockBreakingSample::ECS::BallSystem>(), 7);
        // モデル描画 (優先度 10)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::ModelSystem>(), 10);
        // コリジョンの更新は最後に行う (優先度 12)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::PhysicsSystem>(), 12);

        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        //--------------------------------------------------------------
        // 2Dカメラエンティティの生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity cameraEntity2D = m_scene.CreateEntity();

            // TransformComponent (カメラの位置)
            Tsukino::BuiltIn::ECS::TransformComponent& camTransform2D = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(cameraEntity2D);
            camTransform2D.position                                   = hlslpp::float3(0.0f, 0.0f, -10.0f);    // 手前に引く

            // CameraComponent (投影設定)
            Tsukino::BuiltIn::ECS::CameraComponent& camera2D = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(cameraEntity2D);
            camera2D.projectionType                          = Tsukino::BuiltIn::ECS::CameraComponent::ProjectionType::Orthographic;
            camera2D.orthoSize                               = 720.0f;    // 画面の縦幅を 720 ユニットにする
            camera2D.isPrimary                               = false;     // これをメインカメラにしない
        }

        //--------------------------------------------------------------
        // 3Dカメラエンティティの生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity cameraEntity3D = m_scene.CreateEntity();

            // TransformComponent (カメラの位置)
            Tsukino::BuiltIn::ECS::TransformComponent& camTransform3D = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(cameraEntity3D);
            camTransform3D.position                                   = hlslpp::float3(0.0f, 0.0f, -500.0f);    // 手前に引く

            // CameraComponent (投影設定)
            Tsukino::BuiltIn::ECS::CameraComponent& camera3D = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(cameraEntity3D);
            camera3D.useLookAt                               = true;                                // 注視点を向くようにする
            camera3D.lookAtTarget                            = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 注視点は原点
            camera3D.projectionType                          = Tsukino::BuiltIn::ECS::CameraComponent::ProjectionType::Perspective;
            camera3D.fov                                     = 60.0f;    // 視野角
            camera3D.isPrimary                               = true;     // これをメインカメラにする
        }

        //--------------------------------------------------------------
        // パドル
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity paddleEntity = m_scene.CreateEntity();
            // TransformComponent
            Tsukino::BuiltIn::ECS::TransformComponent& modelTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(paddleEntity);
            modelTransform.position                                   = hlslpp::float3(0.0f, -200.0f, 0.0f);
            modelTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
            modelTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            modelTransform.dirty                                      = true;          // 初回計算のためフラグを立てる
            modelTransform.parent                                     = entt::null;    // 親なし

            // ModelComponent
            Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(paddleEntity);
            model.modelHandle                            = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Models/Paddle.fbx"));
            model.visible                                = true;
            // コリジョンをつける
            Tsukino::BuiltIn::ECS::CollisionComponent& collision = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(paddleEntity);
            collision.extent                                     = {5.0f, 5.0f, 5.0f};    // パドルの当たり判定
            collision.type                                       = Tsukino::BuiltIn::ECS::ColliderType::Box;
            // RBをつける
            Tsukino::BuiltIn::ECS::RigidbodyComponent& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(paddleEntity);
            rb.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Kinematic;
            // パドルコンポーネントを追加
            BlockBreakingSample::ECS::PaddleComponent& paddle = registry.AddComponent<BlockBreakingSample::ECS::PaddleComponent>(paddleEntity);
        }

        //--------------------------------------------------------------
        // ボール
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity ballEntity = m_scene.CreateEntity();
            // TransformComponent
            Tsukino::BuiltIn::ECS::TransformComponent& modelTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(ballEntity);
            modelTransform.position                                   = hlslpp::float3(0.0f, 0.0f, 0.0f);
            modelTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
            modelTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            modelTransform.dirty                                      = true;          // 初回計算のためフラグを立てる
            modelTransform.parent                                     = entt::null;    // 親なし

            // ModelComponent
            Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(ballEntity);
            model.modelHandle                            = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Models/Ball.fbx"));
            model.visible                                = true;
            // コリジョンをつける
            Tsukino::BuiltIn::ECS::CollisionComponent& collision = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(ballEntity);
            collision.extent                                     = {5.0f, 5.0f, 5.0f};    // パドルの当たり判定
            collision.type                                       = Tsukino::BuiltIn::ECS::ColliderType::Box;
            // RBをつける
            Tsukino::BuiltIn::ECS::RigidbodyComponent& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(ballEntity);
            rb.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Kinematic;
            // ボールコンポーネントを追加
            BlockBreakingSample::ECS::BallComponent& paddle = registry.AddComponent<BlockBreakingSample::ECS::BallComponent>(ballEntity);
        }
    }

    //-------------------------------------------------------------
    //! @brief  シーンの更新
    //-------------------------------------------------------------
    void BlockBreakingSampleScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        m_scene.Update(deltaTime);
    }

    //-------------------------------------------------------------
    //! @brief  シーンの終了処理
    //-------------------------------------------------------------
    void BlockBreakingSampleScene::OnExit() {
        // シーン終了時の解放処理などが必要な場合はここに記述します
    }

}    // namespace Tsukino::Sandbox

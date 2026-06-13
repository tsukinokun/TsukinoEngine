//-------------------------------------------------------------
//! @file    FishingGameSampleScene.cpp
//! @brief   ブロック崩しサンプルの実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/Scene/FishingGameSampleScene.hpp>

#ifdef _DEBUG
#include <Tsukino/EngineIntegration/ECS/System/DebugCameraSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraTag.hpp>
#endif

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AudioComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkeletonOutputComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidBodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Serialization/TransformComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/CameraComponentSerialization.hpp>

#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/AudioSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/PhysicsSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/AnimationSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/DirectionalLightSystem.hpp>

#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneManager.hpp>

#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Input/KeyCodes.hpp>

#include <entt/entt.hpp>
#include <hlsl++.h>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    //-------------------------------------------------------------
    //! @brief  シーン固有の初期化処理
    //-------------------------------------------------------------
    void FishingGameSampleScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) {
        // コンテキストをレジストリから取得
        Tsukino::EngineIntegration::EngineContext* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        //-------------------------------------------------------------
        // イベントバスをレジストリから取得
        //-------------------------------------------------------------
        Tsukino::ECS::EventBus& eventBus = m_scene.GetEventBus();

        //--------------------------------------------------------------
        // システムの生成と追加
        //--------------------------------------------------------------
        enum class SystemPriority : int {
            Transform = 0,
            Animation,
            Physics,
            DirectionalLightSystem,
#ifdef _DEBUG
            DebugCamera,
#endif
            Camera,
            Font,
            Render,
            Audio,
        };

        // 登録
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), (int)SystemPriority::Transform);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::AnimationSystem>(), (int)SystemPriority::Animation);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::PhysicsSystem>(eventBus), (int)SystemPriority::Physics);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::DirectionalLightSystem>(), (int)SystemPriority::DirectionalLightSystem);
#ifdef _DEBUG
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::DebugCameraSystem>(), (int)SystemPriority::DebugCamera);
#endif
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), (int)SystemPriority::Camera);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::FontRendererSystem>(), (int)SystemPriority::Font);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SpriteRenderSystem>(), (int)SystemPriority::Render);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::ModelSystem>(), (int)SystemPriority::Render);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::AudioSystem>(), (int)SystemPriority::Audio);

        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        //--------------------------------------------------------------
        // ステージモデルの生成
        //--------------------------------------------------------------
        //--------------------------------------------------------------
        // 地面エンティティの生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity groundEntity = m_scene.CreateEntity();

            // TransformComponent の追加と初期化
            Tsukino::BuiltIn::ECS::TransformComponent& groundTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(groundEntity);
            groundTransform.position                                   = hlslpp::float3(0.0f, -30.0f, 0.0f);
            groundTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
            groundTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);              // 土台
            groundTransform.dirty                                      = true;                                          // 初回計算のためフラグを立てる
            groundTransform.parent                                     = entt::null;                                    // 親なし

            // ModelComponent
            Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(groundEntity);
            model.modelHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/FishingGameSample/Models/Stage.fbx"));
            model.visible     = true;

            // CollisionComponent の追加
            Tsukino::BuiltIn::ECS::CollisionComponent& collision = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(groundEntity);
            collision.extent                                     = {50.0f, 5.0f, 50.0f};    // 土台の当たり判定
            collision.offsetPosition                             = {0.0f, 2.5f, 0.0f};      // 土台の中心にオフセット
            collision.type                                       = Tsukino::BuiltIn::ECS::ColliderType::Box;
            collision.isSensor                                   = false;    // 衝突判定を有効にする

            // RBをつける
            Tsukino::BuiltIn::ECS::RigidbodyComponent& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(groundEntity);
            rb.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Static;
        }

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
            //--------------------------------------------------------------
            // 3Dカメラエンティティの生成
            //--------------------------------------------------------------
            const std::string prefabPath = "Tsukino.Sandbox/Assets/FishingGameSample/Prefabs/3DCamera/Prefab.json";
            entt::entity      testEntity = context->prefabFactory->Instantiate(prefabPath, registry);
        }

        //--------------------------------------------------------------
        // デバッグカメラエンティティの生成 (デバッグビルドのみ)
        //--------------------------------------------------------------
#ifdef _DEBUG
        {
            Tsukino::ECS::Entity debugCamEntity = m_scene.CreateEntity();

            Tsukino::BuiltIn::ECS::TransformComponent& t = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(debugCamEntity);
            t.position                                   = hlslpp::float3(0.0f, 5.0f, -10.0f);
            t.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
            t.dirty                                      = true;

            Tsukino::BuiltIn::ECS::CameraComponent& cam = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(debugCamEntity);
            cam.farZ                                    = 100000.0f;
            cam.isPrimary                               = false;

            Tsukino::BuiltIn::ECS::DebugCameraComponent& debug = registry.AddComponent<Tsukino::BuiltIn::ECS::DebugCameraComponent>(debugCamEntity);
            debug.moveSpeed                                    = 10000.0f;

            registry.AddComponent<Tsukino::BuiltIn::ECS::DebugCameraTag>(debugCamEntity);
        }
#endif

        //--------------------------------------------------------------
        // ディレクショナルライトエンティティの生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity                              lightEntity = m_scene.CreateEntity();
            Tsukino::BuiltIn::ECS::DirectionalLightComponent& light = registry.AddComponent<Tsukino::BuiltIn::ECS::DirectionalLightComponent>(lightEntity);
            light.direction                                         = hlslpp::float3(0.0f, -1.0f, -1.0f);    // 斜め上から照らす
            light.color                                             = hlslpp::float3(1.0f, 1.0f, 1.0f);
            light.intensity                                         = 6.0f;
            light.castShadow                                        = true;
        }
    }

    //-------------------------------------------------------------
    //! @brief  シーンの更新
    //-------------------------------------------------------------
    void FishingGameSampleScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        m_scene.Update(deltaTime);
    }

    //-------------------------------------------------------------
    //! @brief  シーンの終了処理
    //-------------------------------------------------------------
    void FishingGameSampleScene::OnExit() {
        // シーン終了時の解放処理などが必要な場合はここに記述します
    }

}    // namespace Tsukino::Sandbox

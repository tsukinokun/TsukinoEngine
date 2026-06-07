//-------------------------------------------------------------
//! @file    JumpGameSampleScene.cpp
//! @brief   ジャンプゲームサンプルシーンの実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/Scene/JumpGameSampleScene.hpp>

#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/PlatformComponent.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/PlatformGeneratorComponent.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/ScoreComponent.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/ScoreUIComponent.hpp>

#include <Tsukino/Sandbox/JumpGameSample/ECS/System/PlayerSystem.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/System/PlatformSystem.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/System/InGameCameraSystem.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/System/PlatformGeneratorSystem.hpp>
#include <Tsukino/Sandbox/JumpGameSample/ECS/System/ScoreUISystem.hpp>

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
    void JumpGameSampleScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) {
        // コンテキストをレジストリから取得
        Tsukino::EngineIntegration::EngineContext* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();

        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        Tsukino::ECS::EventBus& eventBus = m_scene.GetEventBus();

        registry.SetContext<JumpGameSample::ECS::GameState>(mCurrentState);

        //--------------------------------------------------------------
        // システムの生成と追加
        //--------------------------------------------------------------
        enum class SystemPriority : int {
            Transform = 0,
            Player,
            InGameCamera,
            PlatformGenerator,
            Platform,
            Animation,
            Physics,
            DirectionalLightSystem,
            Camera,
            ScoreUI,
            Font,
            Render,
            Audio,
        };

        // 登録
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), (int)SystemPriority::Transform);
        m_scene.AddSystem(std::make_shared<JumpGameSample::ECS::PlayerSystem>(eventBus), (int)SystemPriority::Player);
        m_scene.AddSystem(std::make_shared<JumpGameSample::ECS::InGameCameraSystem>(), (int)SystemPriority::InGameCamera);
        m_scene.AddSystem(std::make_shared<JumpGameSample::ECS::PlatformGeneratorSystem>(), (int)SystemPriority::PlatformGenerator);
        m_scene.AddSystem(std::make_shared<JumpGameSample::ECS::PlatformSystem>(), (int)SystemPriority::Platform);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::AnimationSystem>(), (int)SystemPriority::Animation);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::PhysicsSystem>(eventBus), (int)SystemPriority::Physics);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::DirectionalLightSystem>(), (int)SystemPriority::DirectionalLightSystem);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), (int)SystemPriority::Camera);
        m_scene.AddSystem(std::make_shared<JumpGameSample::ECS::ScoreUISystem>(), (int)SystemPriority::ScoreUI);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::FontRendererSystem>(), (int)SystemPriority::Font);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SpriteRenderSystem>(), (int)SystemPriority::Render);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::ModelSystem>(), (int)SystemPriority::Render);
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::AudioSystem>(), (int)SystemPriority::Audio);

        //--------------------------------------------------------------
        // Playerエンティティ生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity playerEntity = m_scene.CreateEntity();

            // TransformComponent の追加と初期化
            Tsukino::BuiltIn::ECS::TransformComponent& playerTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(playerEntity);
            playerTransform.position                                   = hlslpp::float3(0.0f, 0.0f, 0.0f);
            playerTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
            playerTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            playerTransform.dirty                                      = true;          // 初回計算のためフラグを立てる
            playerTransform.parent                                     = entt::null;    // 親なし

            // ModelComponent の追加
            Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(playerEntity);
            model.modelHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/JumpGameSample/Models/Arissa.fbx"));
            model.visible     = true;

            // モデルにコリジョンをつける
            Tsukino::BuiltIn::ECS::CollisionComponent& collision = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(playerEntity);
            collision.extent                                     = {35.0f, 70.0f, 70.0f};    // 大きめの当たり判定
            collision.offsetPosition                             = {0.0f, 90.0f, 0.0f};      // モデルの足元から中心にオフセット
            collision.type                                       = Tsukino::BuiltIn::ECS::ColliderType::Capsule;
            collision.isSensor                                   = false;    // 衝突判定と物理的な反発を有効にする

            // RBをつける
            Tsukino::BuiltIn::ECS::RigidbodyComponent& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(playerEntity);
            rb.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Dynamic;
            rb.gravityFactor                              = 9.8f;
            rb.groundCheckDistance                        = 110.0f;
            rb.groundCheckRadius                          = 5.0f;

            // アニメーションを再生・制御するコンポーネント
            Tsukino::BuiltIn::ECS::AnimationPlayerComponent& animPlayer = registry.AddComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(playerEntity);
            animPlayer.current_clip_id = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/JumpGameSample/Anims/Standing Idle.fbx"));
            animPlayer.animation_index = 1;       // 再生するアニメーションのインデックスを指定
            animPlayer.elapsed_time    = 0.0f;    // 0秒からスタート
            animPlayer.playback_speed  = 1.0f;    // 等速再生
            animPlayer.is_looping      = true;    // ループさせる
            animPlayer.is_playing      = true;    // 再生状態にする

            //計算されたボーン行列の出力先（スキニング用）コンポーネント
            Tsukino::BuiltIn::ECS::SkeletonOutputComponent& skeletonOutput =
                registry.AddComponent<Tsukino::BuiltIn::ECS::SkeletonOutputComponent>(playerEntity);

            // プレイヤーコンポーネントをつける
            JumpGameSample::ECS::PlayerComponent& player = registry.AddComponent<JumpGameSample::ECS::PlayerComponent>(playerEntity);

            // アニメーションの制御用コンポーネントをつける
            Tsukino::BuiltIn::ECS::AnimationControllerComponent& animController =
                registry.AddComponent<Tsukino::BuiltIn::ECS::AnimationControllerComponent>(playerEntity);
        }
        //--------------------------------------------------------------
        // PlatformGeneratorエンティティの生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity platformGeneratorEntity = m_scene.CreateEntity();
            // PlatformGeneratorComponent の追加
            JumpGameSample::ECS::PlatformGeneratorComponent& platformGenerator =
                registry.AddComponent<JumpGameSample::ECS::PlatformGeneratorComponent>(platformGeneratorEntity);
        }

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
        Tsukino::ECS::Entity cameraEntity2D = m_scene.CreateEntity();

        // TransformComponent (カメラの位置)
        Tsukino::BuiltIn::ECS::TransformComponent& camTransform2D = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(cameraEntity2D);
        camTransform2D.position                                   = hlslpp::float3(0.0f, 0.0f, -10.0f);    // 手前に引く

        // CameraComponent (投影設定)
        Tsukino::BuiltIn::ECS::CameraComponent& camera2D = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(cameraEntity2D);
        camera2D.projectionType                          = Tsukino::BuiltIn::ECS::CameraComponent::ProjectionType::Orthographic;
        camera2D.orthoSize                               = 720.0f;    // 画面の縦幅を 720 ユニットにする
        camera2D.isPrimary                               = false;     // これをメインカメラにしない

        //--------------------------------------------------------------
        // 3Dカメラエンティティの生成
        //--------------------------------------------------------------
        const std::string prefabPath = "Tsukino.Sandbox/Assets/JumpGameSample/Prefabs/3DCamera/Prefab.json";
        entt::entity      testEntity = context->prefabFactory->Instantiate(prefabPath, registry);

        //--------------------------------------------------------------
        // フォントUIエンティティの生成(ナビ)
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity                       naviEntity    = m_scene.CreateEntity();
            Tsukino::BuiltIn::ECS::TransformComponent& fontTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(naviEntity);
            fontTransform.position                                   = hlslpp::float3(400.0f, 400.0f, 0.0f);
            fontTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
            fontTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            fontTransform.dirty                                      = true;          // 初回計算のためフラグを立てる
            fontTransform.parent                                     = entt::null;    // 親なし

            // FontRendererComponent の追加
            Tsukino::BuiltIn::ECS::FontComponent& font = registry.AddComponent<Tsukino::BuiltIn::ECS::FontComponent>(naviEntity);
            font.text                                  = L"Start Space Key";    // 描画するテキスト
        }

        //--------------------------------------------------------------
        // 得点管理エンティティ
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity                 scoreManagerEntity = m_scene.CreateEntity();
            JumpGameSample::ECS::ScoreComponent& scoreComponent     = registry.AddComponent<JumpGameSample::ECS::ScoreComponent>(scoreManagerEntity);
        }

        //--------------------------------------------------------------
        // 得点用UIエンティティの生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity                       naviEntity    = m_scene.CreateEntity();
            Tsukino::BuiltIn::ECS::TransformComponent& fontTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(naviEntity);
            fontTransform.position                                   = hlslpp::float3(0.0f, 0.0f, 0.0f);
            fontTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
            fontTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            fontTransform.dirty                                      = true;          // 初回計算のためフラグを立てる
            fontTransform.parent                                     = entt::null;    // 親なし

            // FontRendererComponent の追加
            Tsukino::BuiltIn::ECS::FontComponent& font = registry.AddComponent<Tsukino::BuiltIn::ECS::FontComponent>(naviEntity);
            font.text                                  = L"";    // 描画するテキスト

            // ScoreUIComponent をつける
            JumpGameSample::ECS::ScoreUIComponent& scoreUI = registry.AddComponent<JumpGameSample::ECS::ScoreUIComponent>(naviEntity);
        }

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
    void JumpGameSampleScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        m_scene.Update(deltaTime);

        Tsukino::EngineIntegration::EngineContext* context  = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        Tsukino::ECS::Registry&                    registry = m_scene.GetRegistry();

        switch(mCurrentState) {
        case JumpGameSample::ECS::GameState::Ready:
            {
                if(context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Space)) {
                    //-------------------------------------------------------------
                    // シーン変更
                    //-------------------------------------------------------------
                    mCurrentState = JumpGameSample::ECS::GameState::Playing;
                    registry.SetContext<JumpGameSample::ECS::GameState>(mCurrentState);
                    const auto& fontView = registry.View<Tsukino::BuiltIn::ECS::FontComponent>();
                    fontView.each([&](entt::entity entity, Tsukino::BuiltIn::ECS::FontComponent& font) {
                        font.text = L"";    // 描画するテキスト
                    });
                }
            }
            break;
        case JumpGameSample::ECS::GameState::Playing:
            //-------------------------------------------------------------
            // プレイヤーを取得して、TransformComponentのxが0.0fからある程度動いて居ればゲームオーバーにする
            //-------------------------------------------------------------
            {
                auto playerView = registry.View<JumpGameSample::ECS::PlayerComponent, Tsukino::BuiltIn::ECS::TransformComponent>();
                playerView.each([&](entt::entity entity, JumpGameSample::ECS::PlayerComponent& player, Tsukino::BuiltIn::ECS::TransformComponent& transform) {
                    if(abs(transform.position.x) > hlslpp::float1(100.0f)) {
                        const auto& fontView = registry.View<Tsukino::BuiltIn::ECS::FontComponent>();
                        fontView.each([&](entt::entity entity, Tsukino::BuiltIn::ECS::FontComponent& font) {
                            font.text = L"GameOver";    // 描画するテキスト
                        });
                        mCurrentState = JumpGameSample::ECS::GameState::GameOver;
                    }
                });
            }
            break;
        case JumpGameSample::ECS::GameState::GameOver:
            if(context->inputSystem->IsKeyPressed(Tsukino::Input::KeyCode::Space)) {
                context->gameSceneManager->ChangeScene(std::make_unique<Tsukino::Sandbox::JumpGameSampleScene>());
            }
            break;
        }
    }

    //-------------------------------------------------------------
    //! @brief  シーンの終了処理
    //-------------------------------------------------------------
    void JumpGameSampleScene::OnExit() {
    }
}    // namespace Tsukino::Sandbox

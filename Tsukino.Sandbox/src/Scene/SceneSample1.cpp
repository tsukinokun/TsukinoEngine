//-------------------------------------------------------------
//! @file    SceneSample1.cpp
//! @brief   サンプルシーン1の実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/Scene/SampleScene1.hpp>

#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp>

#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Log.hpp>

// 必要なシステムとコンポーネントのインクルード
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/AudioSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AudioComponent.hpp>
#include <Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/EngineIntegration/ECS/System/AnimationSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkeletonOutputComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidBodyComponent.hpp>
#include <Tsukino/EngineIntegration/ECS/System/PhysicsSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Serialization/CameraDescSerialization.hpp>

#include <entt/entt.hpp>
#include <hlsl++.h>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    //-------------------------------------------------------------
    //! @brief  シーン固有の初期化処理
    //-------------------------------------------------------------
    void SampleScene1::OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) {
        // コンテキストをレジストリから取得
        Tsukino::EngineIntegration::EngineContext* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();

        //--------------------------------------------------------------
        // システムの生成と追加
        //--------------------------------------------------------------
        // Transformは一番最初に計算する (優先度 0)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), 0);
        // アニメーションはTransformの後に更新する (優先度 2)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::AnimationSystem>(), 2);
        // カメラは描画前に更新する (優先度 5)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), 5);
        // フォント描画 (優先度 9)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::FontRendererSystem>(), 9);
        // スプライトなど描画用のコマンド生成は後で行う (優先度 10)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SpriteRenderSystem>(), 10);
        // モデル描画 (優先度 10)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::ModelSystem>(), 10);
        // オーディオの更新 (優先度 11)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::AudioSystem>(), 11);
        // コリジョンの更新は最後に行う (優先度 12)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::PhysicsSystem>(), 12);

        //--------------------------------------------------------------
        // アセットのロード
        //--------------------------------------------------------------
        Tsukino::Asset::AssetHandle textureHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Textures/test.jpg"));

        Tsukino::Asset::AssetHandle audioHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Sounds/cat1.wav"));

        Tsukino::Asset::AssetHandle modelHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Models/Arissa.fbx"));

        Tsukino::Asset::AssetHandle animationHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Anims/Typing.fbx"));

        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

        //--------------------------------------------------------------
        // オーディオエンティティの生成
        //--------------------------------------------------------------
        /*       Tsukino::ECS::Entity audioEntity = m_scene.CreateEntity();

        Tsukino::BuiltIn::ECS::AudioComponent& audioComp = registry.AddComponent<Tsukino::BuiltIn::ECS::AudioComponent>(audioEntity);
        audioComp.audioHandle                            = audioHandle;
        audioComp.playOnAwake                            = true;*/

        //--------------------------------------------------------------
        // スプライトエンティティ生成
        //--------------------------------------------------------------
        Tsukino::ECS::Entity entity = m_scene.CreateEntity();

        // TransformComponent の追加と初期化
        Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(entity);
        transform.position                                   = hlslpp::float3(1.0f, 0.0f, 0.0f);
        transform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
        transform.scale                                      = hlslpp::float3(0.5f, 0.5f, 1.0f);
        transform.dirty                                      = true;          // 初回計算のためフラグを立てる
        transform.parent                                     = entt::null;    // 親なし

        // SpriteComponent の追加
        Tsukino::BuiltIn::ECS::SpriteComponent& sprite = registry.AddComponent<Tsukino::BuiltIn::ECS::SpriteComponent>(entity);
        sprite.textureHandle                           = textureHandle;
        sprite.tintColor                               = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);    // 白色
        sprite.uvRect                                  = hlslpp::float4(0.0f, 0.0f, 1.0f, 1.0f);

        //--------------------------------------------------------------
        // Fontエンティティ生成
        //--------------------------------------------------------------
        Tsukino::ECS::Entity fontEntity = m_scene.CreateEntity();

        // TransformComponent の追加と初期化
        Tsukino::BuiltIn::ECS::TransformComponent& fontTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(fontEntity);
        fontTransform.position                                   = hlslpp::float3(0.0f, 0.0f, 0.0f);
        fontTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
        fontTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
        fontTransform.dirty                                      = true;          // 初回計算のためフラグを立てる
        fontTransform.parent                                     = entt::null;    // 親なし

        // FontRendererComponent の追加
        Tsukino::BuiltIn::ECS::FontComponent& font = registry.AddComponent<Tsukino::BuiltIn::ECS::FontComponent>(fontEntity);
        font.text                                  = L"Hello, Tsukino!";    // 描画するテキスト

        //--------------------------------------------------------------
        // Modelエンティティ生成
        //--------------------------------------------------------------
        Tsukino::ECS::Entity modelEntity = m_scene.CreateEntity();

        // TransformComponent の追加と初期化
        Tsukino::BuiltIn::ECS::TransformComponent& modelTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(modelEntity);
        modelTransform.position                                   = hlslpp::float3(0.0f, 0.0f, 0.0f);
        modelTransform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
        modelTransform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
        modelTransform.dirty                                      = true;          // 初回計算のためフラグを立てる
        modelTransform.parent                                     = entt::null;    // 親なし

        // ModelComponent の追加
        Tsukino::BuiltIn::ECS::ModelComponent& model = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(modelEntity);
        model.modelHandle                            = modelHandle;
        model.visible                                = true;

        // モデルにコリジョンをつける
        Tsukino::BuiltIn::ECS::CollisionComponent& collision = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(modelEntity);
        collision.extent                                     = {150.0f, 150.0f, 150.0f};    // 大きめの当たり判定
        collision.type                                       = Tsukino::BuiltIn::ECS::ColliderType::Sphere;

        // RBをつける
        Tsukino::BuiltIn::ECS::RigidbodyComponent& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(modelEntity);
        rb.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Kinematic;

        // アニメーションを再生・制御するコンポーネント
        Tsukino::BuiltIn::ECS::AnimationPlayerComponent& animPlayer = registry.AddComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(modelEntity);
        animPlayer.current_clip_id                                  = animationHandle;    // ロードした testAnim.fbx のハンドルを渡す
        animPlayer.animation_index                                  = 1;                  // 再生するアニメーションのインデックスを指定
        animPlayer.elapsed_time                                     = 0.0f;               // 0秒からスタート
        animPlayer.playback_speed                                   = 1.0f;               // 等速再生
        animPlayer.is_looping                                       = true;               // ループさせる
        animPlayer.is_playing                                       = true;               // 再生状態にする

        // 計算されたボーン行列の出力先（スキニング用）コンポーネント 
        Tsukino::BuiltIn::ECS::SkeletonOutputComponent& skeletonOutput = registry.AddComponent<Tsukino::BuiltIn::ECS::SkeletonOutputComponent>(modelEntity);

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
        // 3Dカメラエンティティの生成（自動生成 ＆ 読込開通テスト版）
        //--------------------------------------------------------------
        Tsukino::ECS::Entity cameraEntity3D = m_scene.CreateEntity();

        // TransformComponent (カメラの位置)
        Tsukino::BuiltIn::ECS::TransformComponent& camTransform3D = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(cameraEntity3D);
        camTransform3D.position                                   = hlslpp::float3(0.0f, 100.0f, 250.0f);

        //--------------------------------------------------------------
        // 双方向シリアライズテスト
        //--------------------------------------------------------------
        const std::string jsonPath = "Tsukino.Sandbox/Assets/Prefabs/Camera3D.json";
        const std::string keyName  = "Camera3D";

        Tsukino::BuiltIn::ECS::CameraDesc cameraDesc;

        //--------------------------------------------------------------
        // まずファイルが存在するかチェック
        //--------------------------------------------------------------
        std::ifstream checkFile(jsonPath);
        if(!checkFile.is_open()) {
            //--------------------------------------------------------------
            // 【1回目】ファイルがないので、デフォルトのカメラ設定を作ってJSONを新規生成
            //--------------------------------------------------------------
            Tsukino::Core::Log::Info("[Prefab Test] JSON file not found. Creating default JSON...");

            cameraDesc.useLookAt      = true;
            cameraDesc.lookAtTarget   = hlslpp::float3(0.0f, 100.0f, 0.0f);
            cameraDesc.projectionType = Tsukino::BuiltIn::ECS::CameraComponent::ProjectionType::Perspective;
            cameraDesc.fov            = 60.0f;
            cameraDesc.isPrimary      = true;

            // JSONファイルを保存（SaveDescのテスト）
            Tsukino::Engine::ECS::Prefab::PrefabFactory::SaveDesc(jsonPath, keyName, cameraDesc);
            Tsukino::Core::Log::Info("[Prefab Test] Default JSON created successfully at: " + jsonPath);
        } else {
            //--------------------------------------------------------------
            // 【2回目以降】ファイルがあるので、一度閉じてからロード
            //--------------------------------------------------------------
            checkFile.close();
            Tsukino::Core::Log::Info("[Prefab Test] JSON file found! Loading data from file...");

            // JSONファイルをロード（LoadDescのテスト）
            cameraDesc = Tsukino::Engine::ECS::Prefab::PrefabFactory::LoadDesc<Tsukino::BuiltIn::ECS::CameraDesc>(jsonPath, keyName);
            Tsukino::Core::Log::Info("[Prefab Test] Load Success! (Loaded FOV: " + std::to_string(cameraDesc.fov) + ")");
        }

        // ロード（または自動生成）したDescから、実行用コンポーネントにパラメータを流し込む
        Tsukino::BuiltIn::ECS::CameraComponent& camera3D = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(cameraEntity3D);
        camera3D.projectionType                          = cameraDesc.projectionType;
        camera3D.orthoSize                               = cameraDesc.orthoSize;
        camera3D.fov                                     = cameraDesc.fov;
        camera3D.aspectRatio                             = cameraDesc.aspectRatio;
        camera3D.nearZ                                   = cameraDesc.nearZ;
        camera3D.farZ                                    = cameraDesc.farZ;
        camera3D.useLookAt                               = cameraDesc.useLookAt;
        camera3D.lookAtTarget                            = cameraDesc.lookAtTarget;
        camera3D.isPrimary                               = cameraDesc.isPrimary;
    }

    //-------------------------------------------------------------
    //! @brief  シーンの更新
    //-------------------------------------------------------------
    void SampleScene1::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        m_scene.Update(deltaTime);
    }

    //-------------------------------------------------------------
    //! @brief  シーンの終了処理
    //-------------------------------------------------------------
    void SampleScene1::OnExit() {
        // シーン終了時の解放処理などが必要な場合はここに記述します
    }

}    // namespace Tsukino::Sandbox

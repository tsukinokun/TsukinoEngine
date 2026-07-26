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
#include <Tsukino/EngineIntegration/ECS/System/PhysicsSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/AnimationSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/DirectionalLightSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SkyAtmosphereSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/DebugCameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/EffectSystem.hpp>

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
#include <Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpringBoneComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraTag.hpp>
#include <Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Serialization/TransformComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/CameraComponentSerialization.hpp>

#include <entt/entt.hpp>
#include <hlsl++.h>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    //-------------------------------------------------------------
    //! @brief  シーン固有の初期化処理
    //-------------------------------------------------------------
    void SampleScene1::OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) {
        //-------------------------------------------------------------
        // コンテキストをレジストリから取得
        //-------------------------------------------------------------
        Tsukino::EngineIntegration::EngineContext* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        //-------------------------------------------------------------
        // イベントバスをレジストリから取得
        //-------------------------------------------------------------
        Tsukino::ECS::EventBus& eventBus = m_scene.GetEventBus();

        //--------------------------------------------------------------
        // クリアカラーを透明に設定
        //--------------------------------------------------------------
        context->renderer->SetClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        //--------------------------------------------------------------
        // システムの生成と追加
        //--------------------------------------------------------------
        // Transformは一番最初に計算する (優先度 0)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), 0);
        // アニメーションはTransformの後に更新する (優先度 2)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::AnimationSystem>(), 2);
#ifdef _DEBUG
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::DebugCameraSystem>(), 4);
#endif
        // カメラは描画前に更新する (優先度 5)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), 5);
        // フォント描画 (優先度 9)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::FontRendererSystem>(), 9);
        // スプライトなど描画用のコマンド生成は後で行う (優先度 10)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SpriteRenderSystem>(), 10);
        // モデル描画 (優先度 10)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::ModelSystem>(), 10);
        // エフェクト描画 (優先度 10)
        {
            auto effectSystem = std::make_shared<Tsukino::BuiltIn::ECS::EffectSystem>();
            m_scene.AddSystem(effectSystem, 10);
            effectSystem->Initialize(m_scene.GetRegistry(), eventBus);
            context->effectSystem = effectSystem.get();
        }
        // オーディオの更新 (優先度 11)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::AudioSystem>(), 11);
        // コリジョンの更新は最後に行う (優先度 12)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::PhysicsSystem>(eventBus), 12);
        // ライトの更新 (優先度 13)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::DirectionalLightSystem>(), 13);
        // スカイアトモスフィアの更新 (優先度 14)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SkyAtmosphereSystem>(), 14);

        //--------------------------------------------------------------
        // アセットのロード
        //--------------------------------------------------------------
        Tsukino::Asset::AssetHandle textureHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Textures/test.jpg"));

        Tsukino::Asset::AssetHandle audioHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Sounds/cat1.wav"));

        Tsukino::Asset::AssetHandle modelHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Models/Test2.fbx"));

        Tsukino::Asset::AssetHandle animationHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Anims/Test2Anim.fbx"));

        Tsukino::Asset::AssetHandle effectHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Effects/Effekseer01/Laser01.efkefc"));

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
        transform.position                                   = hlslpp::float3(500.0f, 0.0f, 0.0f);
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
        model.visible                                = false;

        // モデルにコリジョンをつける
        Tsukino::BuiltIn::ECS::CollisionComponent& collision = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(modelEntity);
        collision.extent                                     = {150.0f, 150.0f, 150.0f};    // 大きめの当たり判定
        collision.type                                       = Tsukino::BuiltIn::ECS::ColliderType::Sphere;

        // RBをつける
        Tsukino::BuiltIn::ECS::RigidbodyComponent& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(modelEntity);
        rb.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Kinematic;

        // アニメーションを再生・制御するコンポーネント
        Tsukino::BuiltIn::ECS::AnimationPlayerComponent& animPlayer = registry.AddComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(modelEntity);
        animPlayer.current_clip_id                                  = animationHandle;    // ロー等速再生ドした testAnim.fbx のハンドルを渡す
        animPlayer.animation_index                                  = 1;                  // 再生するアニメーションのインデックスを指定
        animPlayer.elapsed_time                                     = 2.2f;               // 0秒からスタート
        animPlayer.playback_speed                                   = 0.5f;               //
        animPlayer.is_looping                                       = true;               // ループさせる
        animPlayer.is_playing                                       = true;               // 再生状態にする

        Tsukino::BuiltIn::ECS::SpringBoneComponent& springBone = registry.AddComponent<Tsukino::BuiltIn::ECS::SpringBoneComponent>(modelEntity);

        Tsukino::BuiltIn::ECS::SpringBoneComponent::ChainDef breastL;
        breastL.name                   = "Breast_L";
        breastL.rootNodeName           = "L_breast_01";
        breastL.maxDepth               = 1;
        breastL.settings.stiffness     = 0.4f;     // リアル(0.55)より少し柔らかく、揺れ幅を出す
        breastL.settings.drag          = 0.13f;    // 収まりをやや長めに（2〜3往復くらい残る）
        breastL.settings.inertia       = 0.25f;    // 体の動きに対して、わずかに「置いていかれる」感を演出
        breastL.settings.gravityScale  = 1.0f;
        breastL.settings.angleLimitDeg = 26.0f;
        springBone.chainDefs.push_back(breastL);

        Tsukino::BuiltIn::ECS::SpringBoneComponent::ChainDef breastR;
        breastR.name                   = "Breast_R";
        breastR.rootNodeName           = "R_breast_01";
        breastR.maxDepth               = 1;
        breastR.settings.stiffness     = 0.4f;
        breastR.settings.drag          = 0.13f;
        breastR.settings.inertia       = 0.25f;
        breastR.settings.gravityScale  = 1.0f;
        breastR.settings.angleLimitDeg = 26.0f;
        springBone.chainDefs.push_back(breastR);

        // 計算されたボーン行列の出力先（スキニング用）コンポーネント
        Tsukino::BuiltIn::ECS::SkeletonOutputComponent& skeletonOutput = registry.AddComponent<Tsukino::BuiltIn::ECS::SkeletonOutputComponent>(modelEntity);

        //--------------------------------------------------------------
        // 2Dカメラエンティティの生成
        //--------------------------------------------------------------
        Tsukino::ECS::Entity cameraEntity2D = m_scene.CreateEntity();

        // TransformComponent (カメラの位置)
        Tsukino::BuiltIn::ECS::TransformComponent& camTransform2D = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(cameraEntity2D);
        camTransform2D.position                                   = hlslpp::float3(0.0f, 0.0f, -1.0f);    // 手前に引く

        // CameraComponent (投影設定)
        Tsukino::BuiltIn::ECS::CameraComponent& camera2D = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(cameraEntity2D);
        camera2D.projectionType                          = Tsukino::BuiltIn::ECS::CameraComponent::ProjectionType::Orthographic;
        camera2D.orthoSize                               = 1000.0f;    // 画面の縦幅を 720 ユニットにする
        camera2D.isPrimary                               = false;      // これをメインカメラにしない

        //--------------------------------------------------------------
        //! @brief     PrefabFactory のテスト：PrefabのJSONから3Dカメラエンティティを生成してみる
        //--------------------------------------------------------------
        //Tsukino::Core::Log::Info("=== [PrefabFactory] Instantiate Test Start ===");

        //const std::string instTransformPath = "Tsukino.Sandbox/Assets/Prefabs/TestPrefab_Transform.json";
        //const std::string instCameraPath    = "Tsukino.Sandbox/Assets/Prefabs/TestPrefab_Camera.json";
        const std::string prefabPath = "Tsukino.Sandbox/Assets/Prefabs/TestPrefab.json";

        // 各コンポーネントのJSONを生成（初回のみ）
        //if(!std::filesystem::exists(instTransformPath)) {
        //    Tsukino::BuiltIn::ECS::TransformComponent t{};
        //    t.position = hlslpp::float3(7.0f, 8.0f, 9.0f);
        //    t.rotation = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
        //    t.scale    = hlslpp::float3(1.0f, 1.0f, 1.0f);
        //    context->prefabFactory->Save(instTransformPath, "Transform", t);
        //}

        //if(!std::filesystem::exists(instCameraPath)) {
        //    Tsukino::BuiltIn::ECS::CameraComponent c{};
        //    c.projectionType = Tsukino::BuiltIn::ECS::CameraComponent::ProjectionType::Perspective;
        //    c.fov            = 75.0f;
        //    c.nearZ          = 0.1f;
        //    c.farZ           = 500.0f;
        //    c.isPrimary      = false;
        //    context->prefabFactory->Save(instCameraPath, "Camera", c);
        //}

        //// 目次JSONを生成（初回のみ）
        //if(!std::filesystem::exists(prefabPath)) {
        //    std::map<std::string, std::string> componentList = {
        //        {"Transform", instTransformPath},
        //        {"Camera",    instCameraPath   }
        //    };
        //    std::ofstream             os(prefabPath);
        //    cereal::JSONOutputArchive archive(os);
        //    archive(cereal::make_nvp("Components", componentList));
        //    Tsukino::Core::Log::Info("Created TestPrefab.json");
        //}

        entt::entity testEntity = context->prefabFactory->Instantiate(prefabPath, registry);

        //if(testEntity == entt::null) {
        //    Tsukino::Core::Log::Error("Instantiate FAILED: returned entt::null");
        //} else {
        //    Tsukino::Core::Log::Info("Instantiate OK: entity created");

        //    // Transform の確認（期待値: 7, 8, 9）
        //    auto* t = registry.try_get<Tsukino::BuiltIn::ECS::TransformComponent>(testEntity);
        //    if(t) {
        //        Tsukino::Core::Log::Info("Transform.position.x = " + std::to_string(t->position.x) + " (expect 7)");
        //        Tsukino::Core::Log::Info("Transform.position.y = " + std::to_string(t->position.y) + " (expect 8)");
        //        Tsukino::Core::Log::Info("Transform.position.z = " + std::to_string(t->position.z) + " (expect 9)");
        //    } else {
        //        Tsukino::Core::Log::Error("TransformComponent NOT attached!");
        //    }

        //    // Camera の確認（期待値: fov=75, farZ=500）
        //    auto* c = registry.try_get<Tsukino::BuiltIn::ECS::CameraComponent>(testEntity);
        //    if(c) {
        //        Tsukino::Core::Log::Info("Camera.fov  = " + std::to_string(c->fov) + " (expect 75)");
        //        Tsukino::Core::Log::Info("Camera.farZ = " + std::to_string(c->farZ) + " (expect 500)");
        //    } else {
        //        Tsukino::Core::Log::Error("CameraComponent NOT attached!");
        //    }
        //}

        //Tsukino::Core::Log::Info("=== [PrefabFactory] Instantiate Test End ===");

        //--------------------------------------------------------------
        // デバッグカメラエンティティの生成 (デバッグビルドのみ)
        //--------------------------------------------------------------
#ifdef _DEBUG
        {
            Tsukino::ECS::Entity debugCamEntity = m_scene.CreateEntity();

            Tsukino::BuiltIn::ECS::TransformComponent& t = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(debugCamEntity);
            t.position                                   = hlslpp::float3(0.0f, 150.0f, 50.0f);
            t.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
            t.dirty                                      = true;

            Tsukino::BuiltIn::ECS::CameraComponent& cam = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(debugCamEntity);
            cam.lookAtTarget                            = hlslpp::float3(0.0f, 100.0f, 5.0f);
            cam.nearZ                                   = 1.0f;
            cam.farZ                                    = 10000.0f;
            cam.isPrimary                               = false;

            Tsukino::BuiltIn::ECS::DebugCameraComponent& debug = registry.AddComponent<Tsukino::BuiltIn::ECS::DebugCameraComponent>(debugCamEntity);
            debug.moveSpeed                                    = 1.0f;

            registry.AddComponent<Tsukino::BuiltIn::ECS::DebugCameraTag>(debugCamEntity);
        }
#endif

        //--------------------------------------------------------------
        // ディレクショナルライトエンティティの生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity                              lightEntity = m_scene.CreateEntity();
            Tsukino::BuiltIn::ECS::DirectionalLightComponent& light = registry.AddComponent<Tsukino::BuiltIn::ECS::DirectionalLightComponent>(lightEntity);
            light.direction                                         = hlslpp::float3(0.0f, -0.5f, -1.0f);
            light.color                                             = hlslpp::float3(1.0f, 1.0f, 1.0f);
            light.intensity                                         = 5.0f;
            light.castShadow                                        = true;
        }

        //--------------------------------------------------------------
        // スカイアトモスフィアエンティティの生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity skyEntity = m_scene.CreateEntity();
            registry.AddComponent<Tsukino::BuiltIn::ECS::SkyAtmosphereComponent>(skyEntity);
        }

        //--------------------------------------------------------------
        // エフェクトエンティティの生成
        //--------------------------------------------------------------
        Tsukino::ECS::Entity effectEntity = m_scene.CreateEntity();

        Tsukino::BuiltIn::ECS::TransformComponent& effectTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(effectEntity);
        effectTransform.position                                   = hlslpp::float3(0.0f, 200.0f, 0.0f);
        effectTransform.dirty                                      = true;

        Tsukino::BuiltIn::ECS::EffectComponent& effectComp = registry.AddComponent<Tsukino::BuiltIn::ECS::EffectComponent>(effectEntity);
        effectComp.effectAsset                             = effectHandle;
        effectComp.active                                  = true;
        effectComp.looping                                 = false;
        effectComp.playSpeed                               = 1.0f;

        Tsukino::BuiltIn::ECS::TextureBinding textureBinding;
        textureBinding.texturePath = Tsukino::Core::Path("Assets/Texture/LaserMain01.png");
        textureBinding.layer = 0;
        textureBinding.isOverride = false;
        effectComp.textureBindings.push_back(textureBinding);
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

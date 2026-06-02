//-------------------------------------------------------------
//! @file    JumpGameSampleScene.cpp
//! @brief   ジャンプゲームサンプルシーンの実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/Scene/JumpGameSampleScene.hpp>

#include <Tsukino/Sandbox/JumpGameSample/ECS/Component/PlayerComponent.hpp>

#include <Tsukino/Sandbox/JumpGameSample/ECS/System/PlayerSystem.hpp>

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

#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp>

#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Log.hpp>

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

        //--------------------------------------------------------------
        // システムの生成と追加
        //--------------------------------------------------------------
        // Transformは一番最初に計算する (優先度 0)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), 0);
        // プレイヤーの更新 (優先度 1)
        m_scene.AddSystem(std::make_shared<JumpGameSample::ECS::PlayerSystem>(), 1);
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
        // Modelエンティティ生成
        //--------------------------------------------------------------
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
        collision.extent                                     = {150.0f, 150.0f, 150.0f};    // 大きめの当たり判定
        collision.type                                       = Tsukino::BuiltIn::ECS::ColliderType::Sphere;

        // RBをつける
        Tsukino::BuiltIn::ECS::RigidbodyComponent& rb = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(playerEntity);
        rb.type                                       = Tsukino::BuiltIn::ECS::RigidbodyType::Dynamic;

        // アニメーションを再生・制御するコンポーネント
        Tsukino::BuiltIn::ECS::AnimationPlayerComponent& animPlayer = registry.AddComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(playerEntity);
        animPlayer.current_clip_id = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/JumpGameSample/Anims/Standing Idle.fbx"));
        animPlayer.animation_index = 1;       // 再生するアニメーションのインデックスを指定
        animPlayer.elapsed_time    = 0.0f;    // 0秒からスタート
        animPlayer.playback_speed  = 1.0f;    // 等速再生
        animPlayer.is_looping      = true;    // ループさせる
        animPlayer.is_playing      = true;    // 再生状態にする

        //計算されたボーン行列の出力先（スキニング用）コンポーネント
        Tsukino::BuiltIn::ECS::SkeletonOutputComponent& skeletonOutput = registry.AddComponent<Tsukino::BuiltIn::ECS::SkeletonOutputComponent>(playerEntity);

        // プレイヤーコンポーネントをつける
        JumpGameSample::ECS::PlayerComponent& player = registry.AddComponent<JumpGameSample::ECS::PlayerComponent>(playerEntity);

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
        const std::string prefabPath        = "Tsukino.Sandbox/Assets/JumpGameSample/Prefabs/3DCamera/Prefab.json";
        entt::entity testEntity = context->prefabFactory->Instantiate(prefabPath, registry);
    }

    //-------------------------------------------------------------
    //! @brief  シーンの更新
    //-------------------------------------------------------------
    void JumpGameSampleScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        m_scene.Update(deltaTime);
    }

    //-------------------------------------------------------------
    //! @brief  シーンの終了処理
    //-------------------------------------------------------------
    void JumpGameSampleScene::OnExit() {
    }
}    // namespace Tsukino::Sandbox

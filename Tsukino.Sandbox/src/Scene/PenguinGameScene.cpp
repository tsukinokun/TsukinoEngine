//-------------------------------------------------------------
//! @file    PenguinGameScene.cpp
//! @brief   ペンギンゲームのサンプルシーンの実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/Scene/PenguinGameScene.hpp>

#include <Tsukino/Sandbox/PenguinGame/ECS/Components/PenguinAnimatorComponent.hpp>
#include <Tsukino/Sandbox/PenguinGame/ECS/Systems/PenguinAnimationSystem.hpp>

#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneManager.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DraggableComponent.hpp>
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/InteractionSystem.hpp>

#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Input/KeyCodes.hpp>
#include <Tsukino/Core/Window.hpp>

#include <Tsukino/Renderer/Renderer.hpp>

#include <entt/entt.hpp>
#include <hlsl++.h>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    void PenguinGameScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) {
        // コンテキストをレジストリから取得
        Tsukino::EngineIntegration::EngineContext* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        //-------------------------------------------------------------
        // レジストリを取得
        //-------------------------------------------------------------
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        //-------------------------------------------------------------
        // イベントバスを取得
        //-------------------------------------------------------------
        Tsukino::ECS::EventBus& eventBus = m_scene.GetEventBus();
        //--------------------------------------------------------------
        // クリアカラーを透明に設定
        //--------------------------------------------------------------
        context->renderer->SetClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        //--------------------------------------------------------------
        // このアプリは常にアクティブ
        //--------------------------------------------------------------
        context->window->SetUpdateMode(Tsukino::Core::Window::UpdateMode::AlwaysResident);
        context->window->SetUpdateMode(Tsukino::Core::Window::UpdateMode::ActiveOnly);
        //--------------------------------------------------------------
        // フルスクリーンモードにする
        //--------------------------------------------------------------
        context->window->SetFullscreen(true);

        //--------------------------------------------------------------
        // システムの生成と追加
        //--------------------------------------------------------------
        // enumを使って優先度を管理
        enum class SystemPriority : int {
            Transform        = 0,
            Interaction      = 2,
            Camera           = 5,
            Font             = 9,
            Sprite           = 10,
            PenguinAnimation = 11,
        };

        // Transformは一番最初に計算する
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), static_cast<int>(SystemPriority::Transform));
        // InteractionはTransformの後に計算する
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::InteractionSystem>(), static_cast<int>(SystemPriority::Interaction));
        // カメラは描画前に更新する
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), static_cast<int>(SystemPriority::Camera));
        // フォント描画
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::FontRendererSystem>(), static_cast<int>(SystemPriority::Font));
        // スプライトなど描画用のコマンド生成は後で行う
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SpriteRenderSystem>(), static_cast<int>(SystemPriority::Sprite));
        // ペンギンアニメーションシステムの追加
        m_scene.AddSystem(std::make_shared<PenguinGame::ECS::PenguinAnimationSystem>(), static_cast<int>(SystemPriority::PenguinAnimation));

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
        camera2D.orthoSize                               = context->window->GetHeight();    // ウィンドウの縦ピクセル数を表示幅にする
        camera2D.isPrimary                               = false;                           // これをメインカメラにしない

        //--------------------------------------------------------------
        // ペンギンアニメーターエンティティの生成
        //--------------------------------------------------------------
        Tsukino::ECS::Entity penguinAnimatorEntity = m_scene.CreateEntity();

        auto& animator = registry.AddComponent<PenguinGame::ECS::PenguinAnimatorComponent>(penguinAnimatorEntity);

        // スキン定義のロード（本来はアセット管理システム経由が望ましい）
        // 一旦プロトタイプとして、ここでアセットハンドルをセットします
        auto* assetMgr = context->assetManager;

        // 初期状態のセットアップ
        animator.blinkInterval = 2.0f;    // まばたき間隔

        //--------------------------------------------------------------
        // ペンギンのスキン定義を作成
        //--------------------------------------------------------------
        {
            m_Skin.name          = "Normal";
            m_Skin.centerTexs[0] = assetMgr->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/PenguinGame/penguins/penguin/penguin_center_1.png"));
            m_Skin.centerTexs[1] = assetMgr->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/PenguinGame/penguins/penguin/penguin_center_2.png"));
            m_Skin.leftTexs[0]   = assetMgr->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/PenguinGame/penguins/penguin/penguin_left_1.png"));
            m_Skin.leftTexs[1]   = assetMgr->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/PenguinGame/penguins/penguin/penguin_left_2.png"));
            m_Skin.rightTexs[0]  = assetMgr->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/PenguinGame/penguins/penguin/penguin_right_1.png"));
            m_Skin.rightTexs[1]  = assetMgr->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/PenguinGame/penguins/penguin/penguin_right_2.png"));
            // スキン設定
            animator.currentSkin = &m_Skin;
        }

        //--------------------------------------------------------------
        // センター生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity entity = m_scene.CreateEntity();

            // TransformComponent の追加と初期化
            Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(entity);
            transform.position                                   = hlslpp::float3(100.0f, 100.0f, 0.0f);
            transform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
            transform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            transform.dirty                                      = true;          // 初回計算のためフラグを立てる
            transform.parent                                     = entt::null;    // 親なし

            // SpriteComponent の追加
            Tsukino::BuiltIn::ECS::SpriteComponent& sprite = registry.AddComponent<Tsukino::BuiltIn::ECS::SpriteComponent>(entity);
            sprite.tintColor                               = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);    // 白色
            sprite.uvRect                                  = hlslpp::float4(0.0f, 0.0f, 1.0f, 1.0f);

            animator.centerEntity = entity;    // センターエンティティをアニメーターに登録

            registry.AddComponent<Tsukino::BuiltIn::ECS::DraggableComponent>(entity);
        }

        //--------------------------------------------------------------
        // 左手エンティティ生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity entity = m_scene.CreateEntity();

            // TransformComponent の追加と初期化
            Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(entity);
            transform.position                                   = hlslpp::float3(100.0f, 100.0f, 0.0f);
            transform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
            transform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            transform.dirty                                      = true;          // 初回計算のためフラグを立てる
            transform.parent                                     = entt::null;    // 親なし

            // SpriteComponent の追加
            Tsukino::BuiltIn::ECS::SpriteComponent& sprite = registry.AddComponent<Tsukino::BuiltIn::ECS::SpriteComponent>(entity);
            sprite.tintColor                               = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);    // 白色
            sprite.uvRect                                  = hlslpp::float4(0.0f, 0.0f, 1.0f, 1.0f);
            sprite.sortOrder                               = 1;    // 中央のペンギンより手前に描画

            animator.leftHandEntity = entity;    // 左手エンティティをアニメーターに登録
            registry.AddComponent<Tsukino::BuiltIn::ECS::DraggableComponent>(entity);
        }

        //--------------------------------------------------------------
        // 右手エンティティ生成
        //--------------------------------------------------------------
        {
            Tsukino::ECS::Entity entity = m_scene.CreateEntity();

            // TransformComponent の追加と初期化
            Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(entity);
            transform.position                                   = hlslpp::float3(100.0f, 100.0f, 0.0f);
            transform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
            transform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
            transform.dirty                                      = true;          // 初回計算のためフラグを立てる
            transform.parent                                     = entt::null;    // 親なし

            // SpriteComponent の追加
            Tsukino::BuiltIn::ECS::SpriteComponent& sprite = registry.AddComponent<Tsukino::BuiltIn::ECS::SpriteComponent>(entity);
            sprite.tintColor                               = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);    // 白色
            sprite.uvRect                                  = hlslpp::float4(0.0f, 0.0f, 1.0f, 1.0f);
            sprite.sortOrder                               = 1;    // 中央のペンギンより手前に描画

            animator.rightHandEntity = entity;    // 右手エンティティをアニメーターに登録
            registry.AddComponent<Tsukino::BuiltIn::ECS::DraggableComponent>(entity);
        }
    }

    void PenguinGameScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) {
        m_scene.Update(deltaTime);
    }

    void PenguinGameScene::OnExit() {
        // シーンの終了処理をここに記述
    }
};    // namespace Tsukino::Sandbox

//-------------------------------------------------------------
//! @file    SceneSample1.cpp
//! @brief   サンプルシーン1の実装
//! @author  山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/Scene/SampleScene1.hpp>

#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Core/Path.hpp>

// 必要なシステムとコンポーネントのインクルード
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>

#include <entt/entt.hpp>
#include <hlsl++.h>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    //-------------------------------------------------------------
    //! @brief  シーン固有の初期化処理
    //-------------------------------------------------------------
    void SampleScene1::OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) {
        // コンテキストをレジストリから取得
        auto* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();

        //--------------------------------------------------------------
        // システムの生成と追加
        //--------------------------------------------------------------
        // Transformは一番最初に計算する (優先度 0)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), 0);
        // カメラは描画前に更新する (優先度 5)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), 5);
        // スプライトなど描画用のコマンド生成は後で行う (優先度 10)
        m_scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SpriteRenderSystem>(), 10);

        //--------------------------------------------------------------
        // アセットのロードとエンティティの作成
        //--------------------------------------------------------------
        Tsukino::Asset::AssetHandle textureHandle = context->assets->Load(Tsukino::Core::Path("Assets/Textures/test.jpg"));

        //--------------------------------------------------------------
        // スプライトエンティティ生成
        //--------------------------------------------------------------
        Tsukino::ECS::Entity    entity   = m_scene.CreateEntity();
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();

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
        // カメラエンティティの生成
        //--------------------------------------------------------------
        Tsukino::ECS::Entity cameraEntity = m_scene.CreateEntity();

        // TransformComponent (カメラの位置)
        auto& camTransform    = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(cameraEntity);
        camTransform.position = hlslpp::float3(0.0f, 0.0f, -10.0f);    // 手前に引く

        // CameraComponent (投影設定)
        auto& camera          = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(cameraEntity);
        camera.projectionType = Tsukino::BuiltIn::ECS::CameraComponent::ProjectionType::Orthographic;
        camera.orthoSize      = 720.0f;    // 画面の縦幅を 720 ユニットにする
        camera.isPrimary      = true;      // これをメインカメラにする
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

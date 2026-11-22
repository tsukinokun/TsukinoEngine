//--------------------------------------------------------------
//! @file   RecipeScene.cpp
//! @brief  レシピ: シーンの土台（システムの登録順・カメラ・ライト）と、全レシピをまとめたシーン
//--------------------------------------------------------------
#include <Tsukino/Sandbox/Recipes/RecipeScene.hpp>
#include <Tsukino/Sandbox/Recipes/Recipes.hpp>

#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/EngineIntegration/ECS/System/PhysicsSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/LightSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/AudioSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/LifetimeSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/BuiltInAssets.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Log.hpp>

#include <memory>

// 名前空間 : Tsukino::Sandbox::Recipes
namespace Tsukino::Sandbox::Recipes {

    //--------------------------------------------------------------
    //! @brief 3D の描画と物理に要る組み込みシステムを、正しい順番で登録します。
    //--------------------------------------------------------------
    void AddCoreSystems(Tsukino::ECS::Scene& scene) {
        // 優先度の小さいものから順に毎フレーム回る。
        // 物理は動く剛体の TransformComponent::position を書き換えるので、ワールド行列を作る Transform より前に回す
        // （逆にすると、描画が1フレーム遅れる）。描画用のシステム（Model など）は Camera と Transform の後
        scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::PhysicsSystem>(scene.GetEventBus()), 0);
        scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), 1);
        scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::CameraSystem>(), 2);
        scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::ModelSystem>(), 3);
        scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::LightSystem>(), 4);
        scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::AudioSystem>(), 5);       // AudioComponent を使うときだけ要る
        scene.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::LifetimeSystem>(), 6);    // LifetimeComponent を使うときだけ要る
    }

    //--------------------------------------------------------------
    //! @brief メインカメラ（注視点を見る）とディレクショナルライトを置きます。
    //--------------------------------------------------------------
    void AddCameraAndLight(Tsukino::ECS::Scene& scene, const hlslpp::float3& eye, const hlslpp::float3& target) {
        Tsukino::ECS::Registry& registry = scene.GetRegistry();

        // カメラ。座標系は左手系（+X 右・+Y 上・+Z 奥）。useLookAt なら rotation の代わりに注視点を向く
        Tsukino::ECS::Entity camera = scene.CreateEntity();
        auto&                cameraTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(camera);
        cameraTransform.position             = eye;
        auto& cameraComponent                = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(camera);
        cameraComponent.useLookAt            = true;
        cameraComponent.lookAtTarget         = target;
        cameraComponent.isPrimary            = true;

        // ライトが無いと、通常のモデルは真っ暗になる（自作シェーダーの物はライティングを受けないので関係ない）
        Tsukino::ECS::Entity light = scene.CreateEntity();
        auto&                directional = registry.AddComponent<Tsukino::BuiltIn::ECS::DirectionalLightComponent>(light);
        directional.direction            = hlslpp::float3(0.4f, -1.0f, 0.6f);
        directional.color                = hlslpp::float3(1.0f, 1.0f, 1.0f);
        directional.intensity            = 3.0f;
        directional.castShadow           = true;
    }

    //--------------------------------------------------------------
    //! @brief 全レシピを並べます。
    //--------------------------------------------------------------
    void RecipeScene::OnInitialize(Tsukino::EngineIntegration::EngineAPI& /*api*/) {
        auto* context = m_scene.GetRegistry().GetContext<Tsukino::EngineIntegration::EngineContext*>();
        context->renderer->SetClearColor(0.10f, 0.12f, 0.18f, 1.0f);

        AddCoreSystems(m_scene);
        // ゲーム固有のシステムも同じ AddSystem で足す（描画より前に回すものは小さい優先度に）
        m_scene.AddSystem(std::make_shared<CollisionSoundSystem>(m_scene.GetEventBus(), m_scene.GetRegistry()), 0);
        m_scene.AddSystem(std::make_shared<FollowSystem>(), 0);
        m_scene.AddSystem(std::make_shared<ColorCycleSystem>(), 0);
        m_scene.AddSystem(std::make_shared<SpawnerSystem>(m_scene), 0);

        AddCameraAndLight(m_scene, hlslpp::float3(0.0f, 5.0f, -10.0f), hlslpp::float3(0.0f, 1.0f, 0.0f));
        AddStaticFloor(m_scene, *context, 12.0f);

        // 落ちて床に当たると音が鳴る球と、それを追いかける物
        Tsukino::ECS::Entity sphere = AddDynamicSphere(m_scene, *context, hlslpp::float3(-2.0f, 4.0f, 0.0f), 1.0f);
        Tsukino::ECS::Registry& registry = m_scene.GetRegistry();
        auto&                   sound    = registry.AddComponent<CollisionSoundComponent>(sphere);
        sound.sound                      = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/Sounds/cat1.wav"));

        Tsukino::ECS::Entity follower = m_scene.CreateEntity();
        auto& followerTransform       = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(follower);
        followerTransform.position    = hlslpp::float3(-4.0f, 1.0f, 2.0f);
        followerTransform.scale       = hlslpp::float3(0.3f, 0.3f, 0.3f);
        auto& followerModel           = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(follower);
        followerModel.modelHandle     = context->builtinAssets->models.sphere;
        auto& follow                  = registry.AddComponent<FollowComponent>(follower);
        follow.target.entity          = sphere;    // 参照は Entity をそのまま入れる（Prefab からなら名前で解決される）

        AddColorCycleCube(m_scene, *context, hlslpp::float3(2.0f, 0.5f, 0.0f));

        Tsukino::ECS::Entity spawner = m_scene.CreateEntity();
        auto&                spawnerTransform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(spawner);
        spawnerTransform.position             = hlslpp::float3(0.0f, 3.0f, 3.0f);
        registry.AddComponent<SpawnerComponent>(spawner);

        Tsukino::Core::Log::Info("[Recipe] RecipeScene initialized.");
    }

    //--------------------------------------------------------------
    //! @brief 登録したシステムを回します。
    //--------------------------------------------------------------
    void RecipeScene::OnUpdate(Tsukino::EngineIntegration::EngineAPI& /*api*/, float deltaTime) {
        m_scene.Update(deltaTime);
    }

    //--------------------------------------------------------------
    //! @brief シーンの終了処理（Scene::Shutdown は呼び出し側が行う）。
    //--------------------------------------------------------------
    void RecipeScene::OnExit() {}
}    // namespace Tsukino::Sandbox::Recipes

//--------------------------------------------------------------
//! @file   SceneHelpers.cpp
//! @brief  シーンを作るときの定型を1行で書くための関数の実装
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/Scene/SceneHelpers.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/EngineIntegration/ECS/System/ScriptSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/PhysicsSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/LightSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/AudioSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/LifetimeSystem.hpp>
#include <Tsukino/BuiltIn/BuiltInAssets.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Audio/AudioAsset.hpp>
#include <Tsukino/Audio/AudioManager.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <memory>

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {

    namespace {
        //! コンポーネントを取得します。無ければ付けます。
        template <typename T>
        T& GetOrAdd(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
            return registry.HasComponent<T>(entity) ? registry.GetComponent<T>(entity) : registry.AddComponent<T>(entity);
        }
    }    // namespace

    //--------------------------------------------------------------
    //! @brief 3D ゲームに要る組み込みシステムを、正しい順番でまとめて登録します。
    //--------------------------------------------------------------
    void AddDefaultSystems(Tsukino::ECS::Scene& scene) {
        using namespace Tsukino::BuiltIn::ECS;
        // Script を物理より前に回すと、onUpdate で動かした Kinematic の位置が同じフレームの物理に効く。
        // 物理は Dynamic の位置を書き換えるので、ワールド行列を作る Transform より前（逆だと描画が1フレーム遅れる）
        scene.AddSystem(std::make_shared<ScriptSystem>(scene), -1);
        scene.AddSystem(std::make_shared<PhysicsSystem>(scene.GetEventBus()), 0);
        scene.AddSystem(std::make_shared<TransformSystem>(), 1);
        scene.AddSystem(std::make_shared<CameraSystem>(), 2);
        scene.AddSystem(std::make_shared<ModelSystem>(), 3);
        scene.AddSystem(std::make_shared<LightSystem>(), 4);
        scene.AddSystem(std::make_shared<AudioSystem>(), 5);
        scene.AddSystem(std::make_shared<LifetimeSystem>(), 6);
    }

    //--------------------------------------------------------------
    //! @brief 注視点を見るメインカメラを置きます。
    //--------------------------------------------------------------
    Tsukino::ECS::Entity AddCamera(Tsukino::ECS::Scene& scene, const hlslpp::float3& eye, const hlslpp::float3& target) {
        Tsukino::ECS::Registry& registry = scene.GetRegistry();
        Tsukino::ECS::Entity    camera   = scene.CreateEntity();

        registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(camera).position = eye;
        auto& component        = registry.AddComponent<Tsukino::BuiltIn::ECS::CameraComponent>(camera);
        component.useLookAt    = true;
        component.lookAtTarget = target;
        component.isPrimary    = true;
        return camera;
    }

    //--------------------------------------------------------------
    //! @brief 影を落とすディレクショナルライトを置きます。
    //--------------------------------------------------------------
    Tsukino::ECS::Entity AddDirectionalLight(Tsukino::ECS::Scene& scene, const hlslpp::float3& direction, float intensity) {
        Tsukino::ECS::Entity light = scene.CreateEntity();
        auto& component            = scene.GetRegistry().AddComponent<Tsukino::BuiltIn::ECS::DirectionalLightComponent>(light);
        component.direction        = direction;
        component.color            = hlslpp::float3(1.0f, 1.0f, 1.0f);
        component.intensity        = intensity;
        component.castShadow       = true;
        return light;
    }

    //--------------------------------------------------------------
    //! @brief 基本図形を置きます。
    //--------------------------------------------------------------
    Tsukino::ECS::Entity CreatePrimitive(
        Tsukino::ECS::Scene& scene, PrimitiveShape shape, const hlslpp::float3& position, const hlslpp::float3& scale, PhysicsBody body) {
        Tsukino::ECS::Entity entity = scene.CreateEntity();

        auto& transform    = scene.GetRegistry().AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(entity);
        transform.position = position;
        transform.scale    = scale;

        AttachPrimitive(scene, entity, shape, body);
        return entity;
    }

    //--------------------------------------------------------------
    //! @brief 既にあるエンティティに、基本図形の見た目と物理を付けます。
    //--------------------------------------------------------------
    void AttachPrimitive(Tsukino::ECS::Scene& scene, Tsukino::ECS::Entity entity, PrimitiveShape shape, PhysicsBody body) {
        using namespace Tsukino::BuiltIn::ECS;
        Tsukino::ECS::Registry& registry = scene.GetRegistry();
        EngineContext&          context  = GetEngineContext(scene);

        // Prefab から作ったエンティティには既に付いていることがあるので、あれば上書きする
        const hlslpp::float3 scale = GetOrAdd<TransformComponent>(registry, entity).scale;

        auto& model = GetOrAdd<ModelComponent>(registry, entity);
        switch(shape) {
            case PrimitiveShape::Cube:
                model.modelHandle = context.builtinAssets->models.cube;
                break;
            case PrimitiveShape::Plane:
                model.modelHandle = context.builtinAssets->models.plane;
                break;
            case PrimitiveShape::Sphere:
                model.modelHandle = context.builtinAssets->models.sphere;
                break;
        }

        if(body == PhysicsBody::None)
            return;

        // 当たり判定は Transform の scale を見ないので、ワールドでの半分の大きさを入れる
        auto& collision = GetOrAdd<CollisionComponent>(registry, entity);
        switch(shape) {
            case PrimitiveShape::Cube:
                collision.type   = ColliderType::Box;
                collision.extent = scale * 0.5f;
                break;
            case PrimitiveShape::Sphere: {
                const float radius = std::max({float(scale.x), float(scale.y), float(scale.z)}) * 0.5f;
                collision.type     = ColliderType::Sphere;
                collision.extent   = hlslpp::float3(radius, radius, radius);
                break;
            }
            case PrimitiveShape::Plane: {
                // 厚みの無い当たり判定は速い物がすり抜けるので、厚さ 1 の箱にして上面を平面に合わせる
                constexpr float kThickness = 1.0f;
                collision.type             = ColliderType::Box;
                collision.extent           = hlslpp::float3(float(scale.x) * 0.5f, kThickness * 0.5f, float(scale.z) * 0.5f);
                collision.offsetPosition   = hlslpp::float3(0.0f, -kThickness * 0.5f, 0.0f);
                break;
            }
        }

        auto& rigidbody = GetOrAdd<RigidbodyComponent>(registry, entity);
        switch(body) {
            case PhysicsBody::Static:
                rigidbody.type = RigidbodyType::Static;
                break;
            case PhysicsBody::Dynamic:
                rigidbody.type = RigidbodyType::Dynamic;
                break;
            case PhysicsBody::Kinematic:
                rigidbody.type = RigidbodyType::Kinematic;
                break;
            case PhysicsBody::None:
                break;
        }
    }

    //--------------------------------------------------------------
    //! @brief シーンのエンジンコンテキストを取得します。
    //--------------------------------------------------------------
    EngineContext& GetEngineContext(Tsukino::ECS::Scene& scene) {
        // GameSceneBase::Initialize が OnInitialize より前にレジストリへ入れている
        return *scene.GetRegistry().GetContext<EngineContext*>();
    }

    //--------------------------------------------------------------
    //! @brief アセットを読み込みます。
    //--------------------------------------------------------------
    Tsukino::Asset::AssetHandle LoadAsset(Tsukino::ECS::Scene& scene, const std::string& path) {
        return GetEngineContext(scene).assetManager->Load(Tsukino::Core::Path(path));
    }

    //--------------------------------------------------------------
    //! @brief 読み込んだ音を鳴らします。
    //--------------------------------------------------------------
    bool PlayAudio(Tsukino::ECS::Scene& scene, Tsukino::Asset::AssetHandle sound, float volume, bool loop) {
        EngineContext& context = GetEngineContext(scene);
        const auto     audio   = std::dynamic_pointer_cast<Tsukino::Asset::AudioAsset>(context.assetManager->Get(sound));
        if(!audio) {
            Tsukino::Core::Log::Error("PlayAudio: the handle is not a loaded sound asset. Nothing is played.");
            return false;
        }
        context.audioManager->Play(*audio, loop, volume);
        return true;
    }
}    // namespace Tsukino::EngineIntegration

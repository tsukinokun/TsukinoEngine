//--------------------------------------------------------------
//! @file   PhysicsRecipe.cpp
//! @brief  レシピ: 組み込みモデルで物を置き、当たり判定と重力を付ける
//! @note   要点（詳しくは Recipes/README.md の「物理」）
//!         - 見た目は ModelComponent、当たり判定は CollisionComponent、動き方は RigidbodyComponent
//!         - CollisionComponent::extent は「半分の大きさ」で、Transform の scale を見ない（ワールドの大きさで書く）
//!         - Dynamic の剛体は、PhysicsSystem が毎フレーム TransformComponent::position を物理の位置で上書きする
//!         - 回転は既定ですべての軸が固定（freezeRotationX/Y/Z = true）。転がしたいなら false にする
//!         - 重力は -9.81 単位/秒²（1 単位 = 1 メートル）。PhysicsSystem::SetUnitsPerMeter で変えられる
//--------------------------------------------------------------
#include <Tsukino/Sandbox/Recipes/Recipes.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/BuiltInAssets.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>

// 名前空間 : Tsukino::Sandbox::Recipes
namespace Tsukino::Sandbox::Recipes {

    //--------------------------------------------------------------
    //! @brief 動かない床を置きます（見た目は組み込みの平面、当たり判定は厚みのある箱）。
    //--------------------------------------------------------------
    Tsukino::ECS::Entity AddStaticFloor(Tsukino::ECS::Scene& scene, Tsukino::EngineIntegration::EngineContext& context, float size) {
        constexpr float kThickness = 1.0f;    // 薄い当たり判定は、速く落ちてきた物がすり抜けやすいので厚みを持たせる

        Tsukino::ECS::Registry& registry = scene.GetRegistry();
        Tsukino::ECS::Entity    floor    = scene.CreateEntity();

        auto& transform    = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(floor);
        transform.position = hlslpp::float3(0.0f, 0.0f, 0.0f);
        transform.scale    = hlslpp::float3(size, 1.0f, size);    // 組み込みの平面は 1 x 1 なので scale で広げる

        auto& model       = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(floor);
        model.modelHandle = context.builtinAssets->models.plane;

        // 箱の上面が見た目の平面（Y = 0）と揃うよう、厚さの半分だけ下へずらす
        auto& collision          = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(floor);
        collision.type           = Tsukino::BuiltIn::ECS::ColliderType::Box;
        collision.extent         = hlslpp::float3(size * 0.5f, kThickness * 0.5f, size * 0.5f);
        collision.offsetPosition = hlslpp::float3(0.0f, -kThickness * 0.5f, 0.0f);

        auto& rigidbody = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(floor);
        rigidbody.type  = Tsukino::BuiltIn::ECS::RigidbodyType::Static;
        return floor;
    }

    //--------------------------------------------------------------
    //! @brief 重力で動く球を置きます。
    //--------------------------------------------------------------
    Tsukino::ECS::Entity AddDynamicSphere(Tsukino::ECS::Scene&                       scene,
                                          Tsukino::EngineIntegration::EngineContext& context,
                                          const hlslpp::float3&                      position,
                                          float                                      diameter) {
        Tsukino::ECS::Registry& registry = scene.GetRegistry();
        Tsukino::ECS::Entity    sphere   = scene.CreateEntity();

        auto& transform    = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(sphere);
        transform.position = position;
        transform.scale    = hlslpp::float3(diameter, diameter, diameter);    // 組み込みの球は直径 1

        auto& model       = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(sphere);
        model.modelHandle = context.builtinAssets->models.sphere;

        // 球の当たり判定は extent.x を半径として使う（立方体なら ColliderType::Box で extent = 各辺の半分）
        auto& collision  = registry.AddComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(sphere);
        collision.type   = Tsukino::BuiltIn::ECS::ColliderType::Sphere;
        collision.extent = hlslpp::float3(diameter * 0.5f, diameter * 0.5f, diameter * 0.5f);

        auto& rigidbody         = registry.AddComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(sphere);
        rigidbody.type          = Tsukino::BuiltIn::ECS::RigidbodyType::Dynamic;
        rigidbody.mass          = 1.0f;
        rigidbody.gravityFactor = 1.0f;
        rigidbody.restitution   = 0.3f;    // 0 で跳ねない
        return sphere;
    }
}    // namespace Tsukino::Sandbox::Recipes

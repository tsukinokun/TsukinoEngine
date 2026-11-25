//--------------------------------------------------------------
//! @file   PrefabRecipe.cpp
//! @brief  レシピ: Prefab JSON で複数のエンティティを作り、自作コンポーネントの EntityRef（"#名前"）で互いを参照する
//! @note   JSON は1ファイル（Tsukino.Sandbox/Assets/Prefabs/Recipe/OrbitGroup.json）。形式の説明は GAMEDEV.md
//--------------------------------------------------------------
#include <Tsukino/Sandbox/Recipes/Recipes.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/EngineIntegration/Scene/SceneHelpers.hpp>
#include <Tsukino/EngineIntegration/ECS/Script/ScriptComponent.hpp>
#include <Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/ECS/EntityRef/EntityRef.hpp>
#include <Tsukino/Core/Log.hpp>

#include <cereal/cereal.hpp>

#include <cmath>

// 名前空間 : Tsukino::Sandbox::Recipes
namespace Tsukino::Sandbox::Recipes {
    using namespace Tsukino::EngineIntegration;
    using namespace Tsukino::BuiltIn::ECS;

    //--------------------------------------------------------------
    //! @brief 自作コンポーネント：center の周りを回る（JSON から読む値と、実行中だけの値）
    //--------------------------------------------------------------
    struct OrbitComponent {
        Tsukino::ECS::EntityRef center;          //!< 回る中心（JSON では "#Center" のようにグループ内の名前で書く）
        float                   radius = 1.0f;    //!< 半径
        float                   speed  = 1.0f;    //!< 角速度（ラジアン/秒）
        float                   angle  = 0.0f;    //!< 今の角度（JSON には書かない）
    };

    //--------------------------------------------------------------
    //! @brief cereal 用の読み込み。この load があると、InstantiateGroup の後で EntityRef が実体に解決される
    //! @note  OrbitComponent と同じ名前空間に置く（ADL で見つけるため）
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, OrbitComponent& orbit) {
        archive(cereal::make_nvp("center", orbit.center), cereal::make_nvp("radius", orbit.radius), cereal::make_nvp("speed", orbit.speed));
    }

    //--------------------------------------------------------------
    //! @brief cereal 用の書き出し（PrefabFactory::CaptureEntity で JSON に保存できるようになる）
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const OrbitComponent& orbit) {
        archive(cereal::make_nvp("center", orbit.center), cereal::make_nvp("radius", orbit.radius), cereal::make_nvp("speed", orbit.speed));
    }

    //--------------------------------------------------------------
    //! @brief Prefab JSON のグループから、参照でつながった2体を作ります。
    //--------------------------------------------------------------
    void AddPrefabRecipe(Tsukino::ECS::Scene& scene) {
        Tsukino::Engine::ECS::Prefab::PrefabFactory& factory = *GetEngineContext(scene).prefabFactory;

        // 1. 自作コンポーネントを JSON のキー名で登録する（組み込みは型名で登録済み。同じ名前の2回目は上書き）
        factory.RegisterComponent<OrbitComponent>("OrbitComponent");

        // 2. グループを作る。戻り値は「グループ内の名前 → エンティティ」
        auto entities = factory.InstantiateGroup("Tsukino.Sandbox/Assets/Prefabs/Recipe/OrbitGroup.json", scene.GetRegistry());
        if(!entities.contains("Center") || !entities.contains("Orbiter")) {
            Tsukino::Core::Log::Error("[Recipe] failed to load the prefab group.");
            return;
        }

        // 3. 見た目はコードで付けてもよい（JSON に ModelComponent を書く代わり）
        AttachPrimitive(scene, entities["Center"], PrimitiveShape::Cube);
        AttachPrimitive(scene, entities["Orbiter"], PrimitiveShape::Sphere);

        // 4. 動きは ScriptComponent で。JSON の値は普通のコンポーネントとして読める
        scene.GetRegistry().AddComponent<ScriptComponent>(entities["Orbiter"]).onUpdate = [](ScriptContext& ctx, float deltaTime) {
            OrbitComponent& orbit = ctx.Get<OrbitComponent>();
            if(!ctx.registry.IsValid(orbit.center))    // EntityRef は Entity として使える
                return;
            orbit.angle += orbit.speed * deltaTime;
            const hlslpp::float3 center = ctx.registry.GetComponent<TransformComponent>(orbit.center).position;
            auto&                self   = ctx.Get<TransformComponent>();
            self.position               = center + hlslpp::float3(std::cos(orbit.angle), 0.0f, std::sin(orbit.angle)) * orbit.radius;
            self.dirty                  = true;
        };

        Tsukino::Core::Log::Info("[Recipe] prefab group created (Orbiter.center resolved: " +
                                 std::string(scene.GetRegistry().GetComponent<OrbitComponent>(entities["Orbiter"]).center == entities["Center"] ? "yes" : "no") +
                                 ")");
    }
}    // namespace Tsukino::Sandbox::Recipes

//--------------------------------------------------------------
//! @file   SpawnRecipe.cpp
//! @brief  レシピ: 実行中にエンティティを生成し、時間が来たら破棄する
//! @note   要点（詳しくは Recipes/README.md の「生成と破棄」）
//!         - 生成は Scene::CreateEntity。View で回している最中に同じ種類のコンポーネントを足すと
//!           反復が壊れるので、生成したいものは一度集めて、ループの後で作る
//!         - 破棄は Scene::DestroyEntity（予約。その場では消えず、全システムの更新が終わってから消える）。
//!           システムの中で Registry::DestroyEntity（即時）を呼んではいけない
//!         - 一定時間で消すだけなら LifetimeComponent を付ける（LifetimeSystem の登録が要る）
//--------------------------------------------------------------
#include <Tsukino/Sandbox/Recipes/Recipes.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/BuiltIn/BuiltInAssets.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/LifetimeComponent.hpp>
#include <Tsukino/Core/Log.hpp>

#include <string>
#include <vector>

// 名前空間 : Tsukino::Sandbox::Recipes
namespace Tsukino::Sandbox::Recipes {

    //--------------------------------------------------------------
    //! @brief 間隔が来た生成器の位置へ、小さな立方体を生成します。
    //--------------------------------------------------------------
    void SpawnerSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        using Tsukino::BuiltIn::ECS::TransformComponent;

        struct Request {
            hlslpp::float3 position;
            float          lifetime;
        };
        std::vector<Request> requests;    // 反復中に生成しないよう、先に集める

        auto view = registry.View<SpawnerComponent, TransformComponent>();
        for(auto [entity, spawner, transform] : view.each()) {
            spawner.timer += deltaTime;
            while(spawner.timer >= spawner.interval) {
                spawner.timer -= spawner.interval;
                ++spawner.spawnedCount;
                requests.push_back({transform.position, spawner.lifetime});
            }
        }

        auto* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        for(const Request& request : requests) {
            Tsukino::ECS::Entity spawned = m_scene->CreateEntity();

            auto& transform    = registry.AddComponent<TransformComponent>(spawned);
            transform.position = request.position;
            transform.scale    = hlslpp::float3(0.3f, 0.3f, 0.3f);

            auto& model       = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(spawned);
            model.modelHandle = context->builtinAssets->models.cube;

            // 自分で消すなら m_scene->DestroyEntity(spawned)。時間で消すだけならこれで足りる
            registry.AddComponent<Tsukino::BuiltIn::ECS::LifetimeComponent>(spawned).remaining = request.lifetime;
        }

        if(!requests.empty())
            Tsukino::Core::Log::Info("[Recipe] spawned " + std::to_string(requests.size()) + " entity(s)");
    }
}    // namespace Tsukino::Sandbox::Recipes

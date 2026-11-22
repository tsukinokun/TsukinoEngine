//--------------------------------------------------------------
//! @file   FollowRecipe.cpp
//! @brief  レシピ: 別のエンティティへの参照（EntityRef）をコンポーネントに持たせ、実行時に使う
//! @note   要点（詳しくは Recipes/README.md の「参照」）
//!         - 参照は Tsukino::ECS::EntityRef。コードで作るなら entity に相手の Entity をそのまま入れる
//!         - Prefab JSON の InstantiateGroup で作るなら localName（"#名前"）が生成後に entity へ解決される
//!         - 相手が破棄されていることがあるので、使う前に registry.IsValid で確かめる
//!         - 自作のコンポーネントは普通の struct。システムは ISystem を継承して Scene::AddSystem で足す
//--------------------------------------------------------------
#include <Tsukino/Sandbox/Recipes/Recipes.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <algorithm>

// 名前空間 : Tsukino::Sandbox::Recipes
namespace Tsukino::Sandbox::Recipes {

    //--------------------------------------------------------------
    //! @brief FollowComponent を持つエンティティを、相手の位置へ近づけます。
    //--------------------------------------------------------------
    void FollowSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        using Tsukino::BuiltIn::ECS::TransformComponent;

        auto view = registry.View<FollowComponent, TransformComponent>();
        for(auto [entity, follow, transform] : view.each()) {
            const Tsukino::ECS::Entity target = follow.target.entity;
            if(target == entt::null || !registry.IsValid(target) || !registry.HasComponent<TransformComponent>(target))
                continue;    // 相手が居ない（未設定・破棄済み）

            // position はローカル座標（親なしならワールド座標）。書き換えたら dirty を立てて行列を作り直させる
            const hlslpp::float3 goal = registry.GetComponent<TransformComponent>(target).position + follow.offset;
            const float          t    = std::clamp(follow.speed * deltaTime, 0.0f, 1.0f);
            transform.position        = hlslpp::lerp(transform.position, goal, hlslpp::float3(t, t, t));
            transform.dirty           = true;
        }
    }
}    // namespace Tsukino::Sandbox::Recipes

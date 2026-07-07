//-------------------------------------------------------------
//! @file   DotEatSystem.cpp
//! @brief  DotEatSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/WaterGameSample/ECS/System/DotEatSystem.hpp>

#include <Tsukino/Sandbox/WaterGameSample/ECS/Component/DotComponenet.hpp>
#include <Tsukino/Sandbox/WaterGameSample/ECS/Component/PlayerScoreComponent.hpp>
#include <Tsukino/Sandbox/WaterGameSample/ECS/Component/PlayerMovementComponent.hpp>
#include <Tsukino/Sandbox/WaterGameSample/ECS/Event/DotEatenEvent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/Sandbox/WaterGameSample/ECS/Component/ScoreUIComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>

#include <Tsukino/Core/ECS/Event/EventBus.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <Tsukino/Core/ECS/Event/EventBus.hpp>

#include <hlsl++.h>
#include <vector>

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    //-------------------------------------------------------------
    //! @brief  コンストラクタ
    //-------------------------------------------------------------
    DotEatSystem::DotEatSystem(Tsukino::ECS::EventBus& eventBus)
        : m_eventBus(eventBus) {
    }

    //-------------------------------------------------------------
    //! @brief システムの更新処理
    //-------------------------------------------------------------
    void DotEatSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // 1. プレイヤー（PlayerMovementComponentを持つentity）を1体取得
        //    ※複数プレイヤー対応にしたい場合はループに変更してください
        //-------------------------------------------------------------
        auto playerView = registry.View<PlayerMovementComponent, Tsukino::BuiltIn::ECS::TransformComponent, Tsukino::BuiltIn::ECS::CollisionComponent>();

        Tsukino::ECS::Entity playerEntity = entt::null;
        hlslpp::float3       playerPos    = {0, 0, 0};
        float                playerRadius = 0.0f;

        playerView.each([&](auto entity, auto& /*move*/, auto& tf, auto& col) {
            playerEntity = entity;
            playerPos    = tf.position;
            playerRadius = col.extent.x;    // Sphereコリジョンの半径を利用
        });

        if(playerEntity == entt::null)
            return;

        if(!registry.HasComponent<PlayerScoreComponent>(playerEntity))
            registry.AddComponent<PlayerScoreComponent>(playerEntity);

        auto& score = registry.GetComponent<PlayerScoreComponent>(playerEntity);

        //-------------------------------------------------------------
        // 2. ドットとの距離判定（3D距離。地形の起伏があるためXZだけでなくYも見る）
        //-------------------------------------------------------------
        std::vector<Tsukino::ECS::Entity> eatenDots;

        auto dotView = registry.View<DotComponent, Tsukino::BuiltIn::ECS::TransformComponent>();

        dotView.each([&](auto dotEntity, auto& dot, auto& dotTf) {
            hlslpp::float3 diff   = dotTf.position - playerPos;
            float          distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

            const float dotRadius  = 15.0f;    // DotSpawnerComponent::dotRadiusと合わせる（要調整）
            float       eatRangeSq = (playerRadius + dotRadius) * (playerRadius + dotRadius);

            if(distSq <= eatRangeSq) {
                score.score += dot.scoreValue;

                auto scoreUIView = registry.View<ScoreUIComponent, Tsukino::BuiltIn::ECS::FontComponent>();
                scoreUIView.each([&](auto uiEntity, auto& /*ui*/, auto& font) { font.text = L"Score: " + std::to_wstring(score.score); });

                m_eventBus.Publish(DotEatenEvent{playerEntity, dotEntity, dot.scoreValue});

                eatenDots.push_back(dotEntity);
            }
        });

        //-------------------------------------------------------------
        // 3. 食べたドットを一括削除（イテレータ走査中にDestroyしないための遅延削除）
        //-------------------------------------------------------------
        for(auto dotEntity : eatenDots) {
            registry.DestroyEntity(dotEntity);    // ※実際のRegistry APIに合わせて調整してください
        }
    }

}    // namespace WaterGame::ECS

//-------------------------------------------------------------
//! @file   EnemyBehaviorSystem.cpp
//! @brief  EnemyBehaviorSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/ActionGame/ECS/System/EnemyBehaviorSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/BehaviorTreeComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/EnemyComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/HealthComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <hlsl++.h>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void EnemyBehaviorSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // プレイヤーの位置を取得（単一プレイヤー前提。EnemySystemと同じ方針）
        //-------------------------------------------------------------
        entt::entity   playerEntity   = entt::null;
        hlslpp::float3 playerPosition = hlslpp::float3(0.0f, 0.0f, 0.0f);

        auto playerView = registry.View<PlayerComponent, Tsukino::BuiltIn::ECS::TransformComponent>();
        for(auto entity : playerView) {
            playerEntity   = entity;
            playerPosition = playerView.get<Tsukino::BuiltIn::ECS::TransformComponent>(entity).position;
            break;
        }

        //-------------------------------------------------------------
        // ビヘイビアツリーを持つ敵ごとに黒板を更新し、ツリーを1回進行させる
        //-------------------------------------------------------------
        auto view = registry.View<BehaviorTreeComponent, EnemyComponent, Tsukino::BuiltIn::ECS::TransformComponent>();
        view.each([&](entt::entity                                entity,
                     BehaviorTreeComponent&                      behaviorTree,
                     EnemyComponent&                               enemy,
                     Tsukino::BuiltIn::ECS::TransformComponent& transform) {
            // 死亡している敵は動かさない（破棄はCombatSystemがQueueDestroy経由で行う）
            if(auto* health = registry.try_get<HealthComponent>(entity); health && health->isDead)
                return;

            // 攻撃クールタイムの減算
            if(enemy.attackCooldownTimer > 0.0f) {
                enemy.attackCooldownTimer -= deltaTime;
                if(enemy.attackCooldownTimer < 0.0f)
                    enemy.attackCooldownTimer = 0.0f;
            }

            // 黒板の更新
            EnemyBlackboard& blackboard = behaviorTree.blackboard;
            blackboard.playerEntity      = playerEntity;
            blackboard.hasPlayer         = playerEntity != entt::null;

            if(blackboard.hasPlayer) {
                blackboard.playerPosition = playerPosition;

                hlslpp::float3 toPlayer = playerPosition - transform.position;
                toPlayer.y              = 0.0f;    // 水平面のみで判定する
                blackboard.distanceToPlayer = hlslpp::length(toPlayer);
            } else {
                blackboard.distanceToPlayer = 0.0f;
            }

            // ツリーを1回進行させる
            if(behaviorTree.root) {
                BehaviorContext<EnemyBlackboard> context{registry, entity, blackboard, deltaTime};
                behaviorTree.root->Tick(context);
            }
        });
    }
}    // namespace ActionGame::ECS

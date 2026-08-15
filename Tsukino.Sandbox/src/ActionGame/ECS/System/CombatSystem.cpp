//-------------------------------------------------------------
//! @file   CombatSystem.cpp
//! @brief  CombatSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/ActionGame/ECS/System/CombatSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/WeaponComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/EnemyComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/HealthComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CharacterControllerComponent.hpp>

#include <hlsl++.h>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void CombatSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // 武器：所有者への追従、タイマー更新、攻撃発生時の距離判定ダメージ
        //-------------------------------------------------------------
        auto weaponView = registry.View<WeaponComponent, Tsukino::BuiltIn::ECS::TransformComponent>();
        weaponView.each([&](entt::entity entity, WeaponComponent& weapon, Tsukino::BuiltIn::ECS::TransformComponent& transform) {
            // ボーンアタッチ未実装のため、所有者のTransformへ固定オフセットで追従させる（Phase Bで置き換え）
            if(weapon.owner != entt::null && registry.HasComponent<Tsukino::BuiltIn::ECS::TransformComponent>(weapon.owner)) {
                Tsukino::BuiltIn::ECS::TransformComponent& ownerTransform =
                    registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(weapon.owner);

                hlslpp::float3 rotatedOffset = hlslpp::mul(ownerTransform.rotation, weapon.localOffset);
                transform.position           = ownerTransform.position + rotatedOffset;
                transform.rotation           = ownerTransform.rotation;
                transform.dirty              = true;
            }

            if(weapon.cooldownTimer > 0.0f) {
                weapon.cooldownTimer -= deltaTime;
                if(weapon.cooldownTimer < 0.0f)
                    weapon.cooldownTimer = 0.0f;
            } else if(weapon.attackRequested) {
                weapon.isActive        = true;
                weapon.activeTimer     = weapon.activeDuration;
                weapon.cooldownTimer   = weapon.cooldown;
                weapon.attackRequested = false;

                // 攻撃発生の瞬間、武器の現在位置を中心に範囲内の敵へダメージを与える
                // （1回の攻撃で同じ敵に何度も当たらないよう、判定は発生時の1フレームのみ行う）
                auto enemyView = registry.View<EnemyComponent, Tsukino::BuiltIn::ECS::TransformComponent, HealthComponent>();
                enemyView.each([&](entt::entity                                  enemyEntity,
                                   EnemyComponent&                               enemy,
                                   Tsukino::BuiltIn::ECS::TransformComponent&    enemyTransform,
                                   HealthComponent&                              enemyHealth) {
                    if(enemyHealth.isDead)
                        return;

                    float distance = hlslpp::length(transform.position - enemyTransform.position);
                    if(distance <= weapon.range + enemy.bodyRadius) {
                        enemyHealth.currentHealth -= weapon.damage;
                        if(enemyHealth.currentHealth <= 0.0f) {
                            enemyHealth.currentHealth = 0.0f;
                            enemyHealth.isDead         = true;
                        }
                    }
                });
            }

            if(weapon.isActive) {
                weapon.activeTimer -= deltaTime;
                if(weapon.activeTimer <= 0.0f) {
                    weapon.activeTimer = 0.0f;
                    weapon.isActive    = false;
                }
            }
        });

        //-------------------------------------------------------------
        // プレイヤーを特定する（単一プレイヤー前提）
        //-------------------------------------------------------------
        entt::entity playerEntity = entt::null;
        auto         playerView =
            registry.View<PlayerComponent, Tsukino::BuiltIn::ECS::TransformComponent, HealthComponent>();
        for(auto entity : playerView) {
            playerEntity = entity;
            break;
        }

        //-------------------------------------------------------------
        // 敵：攻撃クールタイム更新、プレイヤーとの距離判定による接触ダメージ
        //-------------------------------------------------------------
        float playerRadius = 35.0f;    // CharacterControllerComponent.radiusと同じ値をフォールバックとして使う
        if(playerEntity != entt::null && registry.HasComponent<Tsukino::BuiltIn::ECS::CharacterControllerComponent>(playerEntity)) {
            playerRadius = registry.GetComponent<Tsukino::BuiltIn::ECS::CharacterControllerComponent>(playerEntity).radius;
        }

        Tsukino::BuiltIn::ECS::TransformComponent* playerTransform = nullptr;
        HealthComponent*                           playerHealth   = nullptr;
        if(playerEntity != entt::null) {
            playerTransform = &registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(playerEntity);
            playerHealth    = &registry.GetComponent<HealthComponent>(playerEntity);
        }

        auto enemyView = registry.View<EnemyComponent, Tsukino::BuiltIn::ECS::TransformComponent, HealthComponent>();
        enemyView.each([&](entt::entity                                  entity,
                           EnemyComponent&                               enemy,
                           Tsukino::BuiltIn::ECS::TransformComponent&    enemyTransform,
                           HealthComponent&                              enemyHealth) {
            //-------------------------------------------------------------
            // 死亡していたら破棄のみ行う
            //-------------------------------------------------------------
            if(enemyHealth.isDead) {
                registry.DestroyEntity(entity);
                return;
            }

            if(enemy.attackTimer > 0.0f) {
                enemy.attackTimer -= deltaTime;
                if(enemy.attackTimer < 0.0f)
                    enemy.attackTimer = 0.0f;
                return;    // クールタイム中は接触判定を行わない
            }

            if(!playerTransform || !playerHealth || playerHealth->isDead)
                return;

            float distance = hlslpp::length(enemyTransform.position - playerTransform->position);
            if(distance <= enemy.bodyRadius + playerRadius) {
                playerHealth->currentHealth -= enemy.contactDamage;
                if(playerHealth->currentHealth <= 0.0f) {
                    playerHealth->currentHealth = 0.0f;
                    playerHealth->isDead         = true;
                }
                enemy.attackTimer = enemy.attackInterval;
            }
        });
    }
}    // namespace ActionGame::ECS

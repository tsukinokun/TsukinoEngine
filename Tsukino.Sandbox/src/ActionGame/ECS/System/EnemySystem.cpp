//-------------------------------------------------------------
//! @file   EnemySystem.cpp
//! @brief  EnemySystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/ActionGame/ECS/System/EnemySystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/EnemyComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <hlsl++.h>
#include <cmath>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void EnemySystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        //-------------------------------------------------------------
        // プレイヤーの位置を取得（単一プレイヤー前提）
        //-------------------------------------------------------------
        entt::entity   playerEntity   = entt::null;
        hlslpp::float3 playerPosition = hlslpp::float3(0.0f, 0.0f, 0.0f);

        auto playerView = registry.View<PlayerComponent, Tsukino::BuiltIn::ECS::TransformComponent>();
        for(auto entity : playerView) {
            playerEntity   = entity;
            playerPosition = playerView.get<Tsukino::BuiltIn::ECS::TransformComponent>(entity).position;
            break;
        }

        if(playerEntity == entt::null)
            return;

        //-------------------------------------------------------------
        // 各敵をプレイヤーに向けて追跡させる
        //-------------------------------------------------------------
        auto enemyView = registry.View<EnemyComponent, Tsukino::BuiltIn::ECS::TransformComponent>();
        enemyView.each([&](entt::entity entity, EnemyComponent& enemy, Tsukino::BuiltIn::ECS::TransformComponent& transform) {
            hlslpp::float3 toPlayer = playerPosition - transform.position;
            toPlayer.y              = 0.0f;    // 水平面のみで追跡する

            float distance = hlslpp::length(toPlayer);
            if(distance <= 0.001f || distance > enemy.detectRange)
                return;

            hlslpp::float3 moveDir = toPlayer / distance;
            transform.position     = transform.position + moveDir * enemy.moveSpeed * deltaTime;

            // 移動方向を向かせる
            float yawRad        = std::atan2(moveDir.x, moveDir.z);
            transform.rotation  = hlslpp::quaternion::rotation_y(yawRad);
            transform.dirty     = true;
        });
    }
}    // namespace ActionGame::ECS

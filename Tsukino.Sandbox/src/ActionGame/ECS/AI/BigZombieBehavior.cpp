//-------------------------------------------------------------
//! @file   BigZombieBehavior.cpp
//! @brief  BigZombie用ビヘイビアツリー構築関数の実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/ActionGame/ECS/AI/BigZombieBehavior.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/EnemyComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/EnemyAnimationSetComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp>

#include <hlsl++.h>
#include <cmath>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    namespace {
        //-------------------------------------------------------------
        //! @brief  "Attack"分岐の条件。攻撃射程内 かつ クールタイム明けか
        //-------------------------------------------------------------
        bool CanAttack(BehaviorContext<EnemyBlackboard>& context) {
            if(!context.blackboard.hasPlayer)
                return false;

            const auto& enemy = context.registry.GetComponent<EnemyComponent>(context.entity);
            return context.blackboard.distanceToPlayer <= enemy.attackRange && enemy.attackCooldownTimer <= 0.0f;
        }

        //-------------------------------------------------------------
        //! @brief  "Chase"分岐の条件。索敵範囲内か
        //-------------------------------------------------------------
        bool CanChase(BehaviorContext<EnemyBlackboard>& context) {
            if(!context.blackboard.hasPlayer)
                return false;

            const auto& enemy = context.registry.GetComponent<EnemyComponent>(context.entity);
            return context.blackboard.distanceToPlayer <= enemy.detectRange;
        }

        //-------------------------------------------------------------
        //! @brief  攻撃アクション。desiredStateをAttackへ書き、再生完了までRunningを返す
        //! @details
        //! 記憶付きSequenceの都合上、Running中はCanAttack（親のCondition）が再評価されない。
        //! そのため終了判定はこの関数自身が持つ：
        //!   - animSet.currentState == Attack かつ AnimationPlayerComponent::is_finished
        //!     （EnemyAnimationSystemが実際にAttackクリップへ切り替え終え、かつ再生が終わった）
        //!   - もしくはattackTimerがattackTimeoutSafetyを超えた（クリップ設定ミス等の保険。
        //!     PlayerAnimationSystemのattackTimeoutSafetyと同じ考え方）
        //-------------------------------------------------------------
        NodeStatus PlayAttack(BehaviorContext<EnemyBlackboard>& context) {
            auto& enemy   = context.registry.GetComponent<EnemyComponent>(context.entity);
            auto& animSet = context.registry.GetComponent<EnemyAnimationSetComponent>(context.entity);

            animSet.desiredState = EnemyAnimState::Attack;
            animSet.attackTimer += context.deltaTime;

            bool isPlayingAttackClip = animSet.currentState == EnemyAnimState::Attack;
            bool clipFinished        = false;
            if(isPlayingAttackClip) {
                const auto& animPlayer = context.registry.GetComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(context.entity);
                clipFinished             = animPlayer.is_finished;
            }

            bool timedOut = animSet.attackTimer >= animSet.attackTimeoutSafety;

            if((isPlayingAttackClip && clipFinished) || timedOut) {
                enemy.attackCooldownTimer = enemy.attackCooldown;
                return NodeStatus::Success;
            }

            return NodeStatus::Running;
        }

        //-------------------------------------------------------------
        //! @brief  追跡アクション。desiredStateをWalkへ書き、プレイヤーへ近づく
        //! @details
        //! EnemySystem::Update（既存の直進追跡）と同じ計算を使う。
        //! attackRange内に到達したらSuccess（攻撃分岐へ譲る）、detectRange外まで
        //! 逃げられたらFailure（記憶付きSequenceの都合上、この関数自身がCanChase相当の
        //! 再判定を持つ必要がある）を返す
        //-------------------------------------------------------------
        NodeStatus MoveToPlayer(BehaviorContext<EnemyBlackboard>& context) {
            if(!context.blackboard.hasPlayer)
                return NodeStatus::Failure;

            auto& enemy     = context.registry.GetComponent<EnemyComponent>(context.entity);
            auto& transform = context.registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(context.entity);
            auto& animSet   = context.registry.GetComponent<EnemyAnimationSetComponent>(context.entity);

            hlslpp::float3 toPlayer = context.blackboard.playerPosition - transform.position;
            toPlayer.y              = 0.0f;    // 水平面のみで追跡する

            float distance = hlslpp::length(toPlayer);
            if(distance > enemy.detectRange)
                return NodeStatus::Failure;    // 見失った

            if(distance <= enemy.attackRange)
                return NodeStatus::Success;    // 射程内に到達。攻撃分岐へ譲る（このフレームは移動しない）

            if(distance > 0.001f) {
                hlslpp::float3 moveDir = toPlayer / distance;
                transform.position     = transform.position + moveDir * enemy.moveSpeed * context.deltaTime;

                // 移動方向を向かせる
                float yawRad       = std::atan2(moveDir.x, moveDir.z);
                transform.rotation = hlslpp::quaternion::rotation_y(yawRad);
                transform.dirty    = true;
            }

            animSet.desiredState = EnemyAnimState::Walk;
            return NodeStatus::Running;
        }

        //-------------------------------------------------------------
        //! @brief  待機アクション。desiredStateをIdleへ書く
        //! @details
        //! 必ずSuccessを返すこと。Runningを返すと記憶付きSelectorがこのIdle分岐に
        //! 貼り付いたままになり、上位のAttack/Chase分岐が二度と評価されなくなる
        //-------------------------------------------------------------
        NodeStatus PlayIdle(BehaviorContext<EnemyBlackboard>& context) {
            auto& animSet         = context.registry.GetComponent<EnemyAnimationSetComponent>(context.entity);
            animSet.desiredState = EnemyAnimState::Idle;
            return NodeStatus::Success;
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief  BigZombie用のビヘイビアツリーを構築する
    //-------------------------------------------------------------
    std::shared_ptr<EnemyBehaviorNode> BuildBigZombieTree() {
        auto attackSequence = std::make_shared<Sequence<EnemyBlackboard>>();
        attackSequence->AddChild(std::make_shared<ConditionNode<EnemyBlackboard>>(&CanAttack));
        attackSequence->AddChild(std::make_shared<ActionNode<EnemyBlackboard>>(&PlayAttack));

        auto chaseSequence = std::make_shared<Sequence<EnemyBlackboard>>();
        chaseSequence->AddChild(std::make_shared<ConditionNode<EnemyBlackboard>>(&CanChase));
        chaseSequence->AddChild(std::make_shared<ActionNode<EnemyBlackboard>>(&MoveToPlayer));

        auto root = std::make_shared<Selector<EnemyBlackboard>>();
        root->AddChild(attackSequence);
        root->AddChild(chaseSequence);
        root->AddChild(std::make_shared<ActionNode<EnemyBlackboard>>(&PlayIdle));

        return root;
    }
}    // namespace ActionGame::ECS

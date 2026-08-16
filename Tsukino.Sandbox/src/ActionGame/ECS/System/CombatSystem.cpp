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
#include <Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/NodeWorldPoseComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/NodeWorldMatrixComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Model/ModelAsset.hpp>
#include <Tsukino/GraphicsCommon/Model/ModelData.hpp>

#include <hlsl++.h>
#include <cmath>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void CombatSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();

        //-------------------------------------------------------------
        // 武器：所有者への追従、タイマー更新、攻撃発生時の距離判定ダメージ
        //-------------------------------------------------------------
        auto weaponView = registry.View<WeaponComponent, Tsukino::BuiltIn::ECS::TransformComponent>();
        weaponView.each([&](entt::entity entity, WeaponComponent& weapon, Tsukino::BuiltIn::ECS::TransformComponent& transform) {
            if(weapon.owner != entt::null && registry.HasComponent<Tsukino::BuiltIn::ECS::TransformComponent>(weapon.owner)) {
                Tsukino::BuiltIn::ECS::TransformComponent& ownerTransform =
                    registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(weapon.owner);

                // 所有者のアタッチ対象ボーン名を解決してキャッシュする。
                // NodeWorldPoseComponentはAnimationSystemが「現在再生中のアニメーションクリップ」の
                // modelData.nodesを基準に書き出すため（プレイヤー本体のModelComponentのモデルとは
                // 別アセットで、ノード構成が一致するとは限らない）、解決もクリップ側のノード一覧に対して行う。
                // Idle/Run/Attack等、クリップごとに別アセット＝別のnode配列なので、一度解決したインデックスを
                // 使い回すと別クリップ再生中に誤ったノードを指してしまう。再生中のクリップが変わるたびに
                // 再解決する（クリップ未ロード等で解決できない場合は、ロードされるまで毎フレーム再試行する）。
                if(ctx && ctx->assetManager
                   && registry.HasComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(weapon.owner)) {
                    auto& ownerAnim = registry.GetComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(weapon.owner);
                    if(weapon.resolvedAgainstClip != ownerAnim.current_clip_id) {
                        auto asset = ctx->assetManager->Get(ownerAnim.current_clip_id);
                        if(asset && asset->GetType() == Tsukino::Asset::AssetType::Model) {
                            auto modelAss           = std::static_pointer_cast<Tsukino::Asset::ModelAsset>(asset);
                            weapon.handBoneNodeIndex = UINT32_MAX;
                            for(u32 i = 0; i < modelAss->modelData.nodes.size(); ++i) {
                                if(modelAss->modelData.nodes[i].name == weapon.handBoneName) {
                                    weapon.handBoneNodeIndex = i;
                                    break;
                                }
                            }
                            weapon.resolvedAgainstClip = ownerAnim.current_clip_id;    // アセットが読めた時点で確定（見つからなければUINT32_MAXのまま）
                        }
                    }
                }

                // isAttackingがfalse/trueへ瞬時に切り替わっても追従先の「目標」自体が不連続にジャンプ
                // しないよう、0(非攻撃)↔1(攻撃)の連続値attackBlendを指数減衰で追従させ、
                // 追従パラメータはこのattackBlendで連続的に補間する（末尾のexp減衰補間は目標への
                // 追従を滑らかにするだけで、目標自体の飛びは吸収しきれず「カクッ」とスナップして
                // 見える不具合の原因だったため、目標を作る側で先に連続化する）
                float attackBlendTarget = weapon.isAttacking ? 1.0f : 0.0f;
                float attackBlendLerpT  = 1.0f - std::exp(-weapon.attackBlendSpeed * deltaTime);
                weapon.attackBlend += (attackBlendTarget - weapon.attackBlend) * attackBlendLerpT;

                // attackBlendが1に近いほどattackHandTrackingWeight/attackLocalOffset/attackGripRotationOffsetへ、
                // 0に近いほど通常値へ連続的に補間する。
                // localOffsetは「ほぼ静止した基準点からの浮遊位置」として調整された大きい値（170ユニット近く）
                // なので、実際に振られる手ボーンにそのまま適用するとテコの原理で武器が大きく・速く振り回されてしまう
                float              trackingWeight = weapon.handTrackingWeight + (weapon.attackHandTrackingWeight - weapon.handTrackingWeight) * weapon.attackBlend;
                hlslpp::float3     gripOffset     = hlslpp::lerp(weapon.localOffset, weapon.attackLocalOffset, weapon.attackBlend);
                hlslpp::quaternion gripRotOffset = hlslpp::slerp(weapon.gripRotationOffset, weapon.attackGripRotationOffset, weapon.attackBlend);

                // ボーンが解決できていれば手のボーンへアタッチする。できなければ従来通り
                // ルートTransformへ固定オフセットで追従させる（フォールバック）。
                // ここではtransformへ直接書き込まず、まず目標位置・姿勢を求める
                // （攻撃モーションへの出入りでgripOffset/trackingWeightが瞬時に切り替わっても
                //   目標そのものが動くだけで、実際の反映は末尾のexp減衰補間に任せるため）
                hlslpp::float3     targetPosition = transform.position;
                hlslpp::quaternion targetRotation = transform.rotation;

                bool attachedToBone = false;
                if(weapon.handBoneNodeIndex != UINT32_MAX
                   && registry.HasComponent<Tsukino::BuiltIn::ECS::NodeWorldMatrixComponent>(weapon.owner)) {
                    auto& ownerMatrices = registry.GetComponent<Tsukino::BuiltIn::ECS::NodeWorldMatrixComponent>(weapon.owner);
                    if(weapon.handBoneNodeIndex < ownerMatrices.matrices.size()) {
                        // Unityのボーンソケットと同じ考え方：実際にスキンメッシュを描画するのに使う
                        // スケール込みのボーン行列（globalNodeMatrices由来）から位置・回転を取り出す。
                        // NodeWorldPoseComponent（スケール1近似の軽量版。揺れ物物理専用）は使わない
                        // ——これが攻撃中に武器が暴れる不具合の原因だった。
                        hlslpp::float3     handBonePos;
                        hlslpp::quaternion handBoneRot;
                        Tsukino::Core::Math::matrix::decomposePositionRotation(
                            ownerMatrices.matrices[weapon.handBoneNodeIndex], handBonePos, handBoneRot);

                        // モデルローカルのボーン姿勢 → ワールド空間（所有者のTransformを反映）
                        hlslpp::float3 handWorldPos =
                            ownerTransform.position + hlslpp::mul(handBonePos * ownerTransform.scale, ownerTransform.rotation);
                        hlslpp::quaternion handWorldRot = hlslpp::mul(handBoneRot, ownerTransform.rotation);

                        // trackingWeightで手ボーン姿勢への追従度を位置・回転の両方に一貫して適用する
                        // （アニメーションクリップのボーン姿勢が信頼できない/振り幅が大きい場合に下げて使う。
                        //   0にすると所有者のルートTransformにのみ追従する）
                        hlslpp::float3     worldPos = hlslpp::lerp(ownerTransform.position, handWorldPos, trackingWeight);
                        hlslpp::quaternion worldRot = hlslpp::slerp(ownerTransform.rotation, handWorldRot, trackingWeight);

                        // 握り位置・向きの微調整（WeaponComponentのオフセットをボーンローカル空間で適用）
                        targetPosition = worldPos + hlslpp::mul(gripOffset, worldRot);
                        targetRotation = hlslpp::mul(gripRotOffset, worldRot);
                        attachedToBone  = true;
                    }
                }

                if(!attachedToBone) {
                    hlslpp::float3 rotatedOffset = hlslpp::mul(ownerTransform.rotation, gripOffset);
                    targetPosition                = ownerTransform.position + rotatedOffset;
                    targetRotation                = hlslpp::mul(gripRotOffset, ownerTransform.rotation);
                }

                // 手に持つのではなく所有者の周りをふわふわ浮遊させる演出
                // （旋回はさせず、localOffsetの位置を基準に上下・左右前後へゆったり漂わせ、
                //   姿勢もわずかに前後へ傾けるだけに留めてほぼ縦向きを保つ）。
                // isAttackingでの完全な有効/無効切り替えはやめ、常に浮遊姿勢・漂いを計算した上で
                // attackBlendによりボーン追従の姿勢と連続的にブレンドする（攻撃開始/終了の瞬間に
                // 回転が浮遊姿勢↔ボーン追従で不連続に入れ替わってスナップして見える不具合の原因だったため）
                if(weapon.floatEnabled) {
                    weapon.floatTime += deltaTime;

                    // 姿勢はほぼ縦向きを保ったまま、わずかに前後・左右へ揺れるだけ（旋回はしない）。
                    // gripRotationOffsetは「手に持つ」ときの握り角度調整用のオフセットで、
                    // モデル自体がエクスポート時点で既に縦向き（Y-up）のため、浮遊時には適用しない
                    float               swayX = std::sin(weapon.floatTime * weapon.floatSwaySpeed) * weapon.floatSwayAngle;
                    float               swayZ = std::cos(weapon.floatTime * weapon.floatSwaySpeed * 0.8f) * weapon.floatSwayAngle;
                    hlslpp::quaternion floatRotation = hlslpp::mul(hlslpp::quaternion::rotation_x(swayX), hlslpp::quaternion::rotation_z(swayZ));

                    // attackBlendが1に近いほどボーン追従の姿勢（targetRotation）、0に近いほど浮遊姿勢へ
                    targetRotation = hlslpp::slerp(floatRotation, targetRotation, weapon.attackBlend);

                    // 上下方向の漂い（ワールドYはどの向きでも共通なのでそのまま加算）。
                    // attackBlendが1に近づくほど自然にゼロへ収束させる
                    float bobOffset = std::sin(weapon.floatTime * weapon.floatBobSpeed) * weapon.floatBobAmplitude * (1.0f - weapon.attackBlend);
                    targetPosition.y += bobOffset;

                    // 左右・前後方向の漂い。所有者のローカル空間で計算してからownerの向きで回転することで、
                    // 所有者に対する相対的な漂い方がどの向きでも同じになるようにする（こちらもattackBlendでゼロへ収束）
                    hlslpp::float3 localDrift(std::sin(weapon.floatTime * weapon.floatDriftSpeed) * weapon.floatDriftAmplitude,
                                              0.0f,
                                              std::cos(weapon.floatTime * weapon.floatDriftSpeed * 0.7f) * weapon.floatDriftAmplitude);
                    targetPosition += hlslpp::mul(localDrift, ownerTransform.rotation) * (1.0f - weapon.attackBlend);
                }

                // 攻撃モーションへの出入りやフォールバック切り替えで目標位置・姿勢が
                // 瞬時に変わっても瞬間移動しないよう、指数減衰で現在値から追従させる
                // （PlayerSystem.cppの向き直し補間と同じ手法。フレームレート非依存）
                float positionLerpT = 1.0f - std::exp(-weapon.attachPositionLerpSpeed * deltaTime);
                float rotationLerpT = 1.0f - std::exp(-weapon.attachRotationLerpSpeed * deltaTime);
                transform.position   = hlslpp::lerp(transform.position, targetPosition, positionLerpT);
                transform.rotation   = hlslpp::slerp(transform.rotation, targetRotation, rotationLerpT);
                transform.dirty       = true;
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

                    // 高さ方向のずれに判定が左右されないよう、水平（XZ）距離のみで判定する
                    hlslpp::float3 toEnemy = transform.position - enemyTransform.position;
                    toEnemy.y              = 0.0f;
                    float distance         = hlslpp::length(toEnemy);
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
                // view の反復中なので即時破棄してはならない（イテレータが壊れる）。
                // 予約だけ行い、実際の破棄は Scene::Update() が全 System の更新後に行う。
                registry.QueueDestroy(entity);
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

            // 高さ方向のずれ（プレイヤー/敵モデルの原点位置の違いなど）に接触判定が
            // 左右されないよう、水平（XZ）距離のみで判定する
            hlslpp::float3 toPlayer = enemyTransform.position - playerTransform->position;
            toPlayer.y              = 0.0f;
            float distance          = hlslpp::length(toPlayer);
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

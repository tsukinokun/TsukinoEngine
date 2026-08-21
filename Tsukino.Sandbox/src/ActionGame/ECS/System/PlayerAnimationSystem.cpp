//-------------------------------------------------------------
//! @file   PlayerAnimationSystem.cpp
//! @brief  PlayerAnimationSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/ActionGame/ECS/System/PlayerAnimationSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerAnimationSetComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/WeaponComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/CharacterControllerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp>

#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Core/Log.hpp>

#include <hlsl++.h>
#include <algorithm>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    namespace {
        //! @brief ステート切り替え時のデフォルトクロスフェード時間（秒）
        constexpr float kAnimBlendTime = 0.15f;

        //-------------------------------------------------------------
        //! @brief  「指定クリップへクロスフェードする」OnEnterコールバックを作るヘルパー
        //! @param  clipMember     [in] PlayerAnimationSetComponentが持つクリップハンドルへのメンバポインタ
        //! @param  animationIndex [in] AnimationControllerComponentへ渡す再生インデックス
        //! @param  looping        [in] ループ再生するか
        //! @param  fadeTime       [in] クロスフェードにかける時間（秒）
        //-------------------------------------------------------------
        StateMachine<PlayerAnimState>::Callback MakeClipEnterCallback(Tsukino::Asset::AssetHandle PlayerAnimationSetComponent::* clipMember,
                                                                        u32                                                        animationIndex,
                                                                        bool                                                       looping,
                                                                        float                                                      fadeTime = kAnimBlendTime,
                                                                        bool                                                       inPlace  = false) {
            return [clipMember, animationIndex, looping, fadeTime, inPlace](Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
                Tsukino::Asset::AssetHandle clip = registry.GetComponent<PlayerAnimationSetComponent>(entity).*clipMember;
                if(!clip.IsValid())
                    return;

                auto& animController = registry.GetComponent<Tsukino::BuiltIn::ECS::AnimationControllerComponent>(entity);

                animController.next.clip            = clip;
                animController.next.animation_index = animationIndex;
                animController.next.fade_time       = fadeTime;
                animController.next.immediate       = false;    // クロスフェードで切り替える
                animController.next.is_looping      = looping;
                animController.next.clip_start_time = 0.0f;     // クリップ全体を再生（部分再生はAttack1-3のみ使う）
                animController.next.clip_end_time   = 0.0f;
                animController.next.in_place        = inPlace;

                // 攻撃段のAttackStep::playbackSpeedが前段から持ち越されないよう、
                // 攻撃以外のステートへ入るときは必ず等速へ戻す
                registry.GetComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(entity).playback_speed = 1.0f;
            };
        }

        //-------------------------------------------------------------
        //! @brief  「連撃のN段目へ入る」OnEnterコールバックを作るヘルパー
        //! @param  stepIndex [in] PlayerAnimationSetComponent::attackSteps のインデックス
        //-------------------------------------------------------------
        StateMachine<PlayerAnimState>::Callback MakeAttackStepEnterCallback(u32 stepIndex) {
            return [stepIndex](Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
                auto&       animSet = registry.GetComponent<PlayerAnimationSetComponent>(entity);
                const auto& step    = animSet.attackSteps[stepIndex];
                if(!step.clip.IsValid())
                    return;

                auto& animController = registry.GetComponent<Tsukino::BuiltIn::ECS::AnimationControllerComponent>(entity);

                animController.next.clip            = step.clip;
                animController.next.animation_index = step.animationIndex;
                animController.next.fade_time       = step.fadeTime;
                animController.next.immediate       = false;    // クロスフェードで切り替える
                animController.next.is_looping      = false;    // 各段は単発再生（コンボ窓を過ぎたら次段かIdle等へ抜ける）
                animController.next.clip_start_time = step.startTime;
                animController.next.clip_end_time   = step.endTime;
                animController.next.in_place        = step.inPlace;

                // この段の再生速度倍率を適用する（攻撃モーションの速さの微調整用）。
                // MakeClipEnterCallback側で他ステートに入るときに1.0へ戻すため、ここでの
                // 変更が移動/待機アニメーションへ漏れることはない
                registry.GetComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(entity).playback_speed = step.playbackSpeed;

                animSet.attackComboIndex = stepIndex;
                animSet.attackTimer      = 0.0f;

                //-------------------------------------------------------------
                // 武器の当たり判定を再アームする。WeaponComponent::cooldown（単発攻撃前提の値）が
                // コンボ窓キャンセルのタイミングでまだ残っている可能性があるため、段の突入時に
                // 明示的に0へ落として次段のヒット判定をブロックしないようにする
                //-------------------------------------------------------------
                auto& player = registry.GetComponent<PlayerComponent>(entity);
                if(player.weaponEntity != entt::null && registry.HasComponent<WeaponComponent>(player.weaponEntity)) {
                    auto& weapon            = registry.GetComponent<WeaponComponent>(player.weaponEntity);
                    weapon.attackRequested = true;
                    weapon.cooldownTimer   = 0.0f;
                    weapon.nextActiveDurationOverride = step.hitWindowDuration;
                }
            };
        }

        //-------------------------------------------------------------
        //! @brief  0-basedの段インデックスを対応するPlayerAnimStateへ変換する
        //-------------------------------------------------------------
        PlayerAnimState AttackStateFromIndex(u32 index) {
            switch(index) {
            case 0: return PlayerAnimState::Attack1;
            case 1: return PlayerAnimState::Attack2;
            default: return PlayerAnimState::Attack3;
            }
        }

        //-------------------------------------------------------------
        //! @brief  指定ステートが連撃中（Attack1-3のいずれか）かを判定する
        //-------------------------------------------------------------
        bool IsAttackState(PlayerAnimState state) {
            return state == PlayerAnimState::Attack1 || state == PlayerAnimState::Attack2 || state == PlayerAnimState::Attack3;
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief コンストラクタ。各ステートのOnEnterコールバック（クリップ切り替え）を登録する
    //-------------------------------------------------------------
    PlayerAnimationSystem::PlayerAnimationSystem() {
        // これらのMixamo由来のFBXは、いずれもindex 0が「Armature」レイヤーの1tickのスタブ、
        // index 1が実際の全ボーンモーション（52チャンネル）になっているため、再生には1を指定する
        m_stateMachine.RegisterState(PlayerAnimState::Idle, MakeClipEnterCallback(&PlayerAnimationSetComponent::idleClip, 1, true));
        m_stateMachine.RegisterState(PlayerAnimState::Run, MakeClipEnterCallback(&PlayerAnimationSetComponent::runClip, 1, true));
        m_stateMachine.RegisterState(PlayerAnimState::FastRun, MakeClipEnterCallback(&PlayerAnimationSetComponent::fastRunClip, 1, true));
        m_stateMachine.RegisterState(PlayerAnimState::Jump, MakeClipEnterCallback(&PlayerAnimationSetComponent::jumpClip, 1, false));
        // 連撃の各段は、Weapon Attack.fbx 1本を時間レンジで3分割して参照する（PlayerAnimationSetComponent::attackSteps）
        m_stateMachine.RegisterState(PlayerAnimState::Attack1, MakeAttackStepEnterCallback(0));
        m_stateMachine.RegisterState(PlayerAnimState::Attack2, MakeAttackStepEnterCallback(1));
        m_stateMachine.RegisterState(PlayerAnimState::Attack3, MakeAttackStepEnterCallback(2));
    }

    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void PlayerAnimationSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto view = registry.View<PlayerComponent,
                                  Tsukino::BuiltIn::ECS::CharacterControllerComponent,
                                  PlayerAnimationSetComponent,
                                  Tsukino::BuiltIn::ECS::AnimationPlayerComponent>();
        view.each([&](entt::entity                                          entity,
                     PlayerComponent&                                      player,
                     Tsukino::BuiltIn::ECS::CharacterControllerComponent& cc,
                     PlayerAnimationSetComponent&                          animSet,
                     Tsukino::BuiltIn::ECS::AnimationPlayerComponent&     animPlayer) {
            bool isAttacking = IsAttackState(animSet.currentState);

            if(isAttacking) {
                animSet.attackTimer += deltaTime;
            }

            //-------------------------------------------------------------
            // 左クリックの生入力（PlayerComponent::attackInputPressed）を消費する。
            // 攻撃中でなければ即座に1段目を開始し、攻撃中なら現在の段が終わるまで
            // 1回だけ先行入力としてバッファする（連打しても2段先へは飛ばない）。
            // バッファされた入力は現在の段を最後まで再生し終えてから消費するため、
            // 再生中のモーションが途中でキャンセルされることはない
            //-------------------------------------------------------------
            bool attackJustPressed     = player.attackInputPressed;
            player.attackInputPressed = false;

            bool hasWeapon = player.weaponEntity != entt::null && registry.HasComponent<WeaponComponent>(player.weaponEntity);

            bool attackJustStarted = false;
            if(attackJustPressed && hasWeapon) {
                if(!isAttacking) {
                    attackJustStarted = true;
                } else {
                    animSet.attackInputBuffered = true;
                }
            }

            //-------------------------------------------------------------
            // 現在の段の終了判定は、原則としてAnimationPlayerComponent::is_finished
            // （実クリップの再生完了）で行う。attackTimeoutSafetyはクリップ設定ミス等の保険
            // （詳細はPlayerAnimationSetComponent::attackTimeoutSafetyのコメント参照）
            //-------------------------------------------------------------
            bool attackStepFinished = isAttacking && (animPlayer.is_finished || animSet.attackTimer >= animSet.attackTimeoutSafety);

            //-------------------------------------------------------------
            // 現在のプレイヤーの状態から、あるべきアニメーションステートを決定する
            // （移動・ジャンプの判定はまず素で計算し、攻撃継続/開始/コンボ進行が
            //   優先される場合はその後で上書きする）
            //-------------------------------------------------------------
            float moveInputLen = hlslpp::length(cc.moveInput);

            PlayerAnimState desiredState;
            if(!cc.isGrounded) {
                desiredState = PlayerAnimState::Jump;
            } else if(moveInputLen > 1.0f) {
                desiredState = player.isSprinting ? PlayerAnimState::FastRun : PlayerAnimState::Run;
            } else {
                desiredState = PlayerAnimState::Idle;
            }

            if(attackJustStarted) {
                desiredState = PlayerAnimState::Attack1;
            } else if(isAttacking && !attackStepFinished) {
                desiredState = animSet.currentState;    // 現在の段を最後まで再生する（キャンセルしない）
            } else if(attackStepFinished) {
                bool isLastStep = animSet.attackComboIndex + 1 >= PlayerAnimationSetComponent::kAttackComboCount;
                if(animSet.attackInputBuffered && !isLastStep) {
                    // 先行入力があれば次段へ連撃継続する
                    animSet.attackInputBuffered = false;
                    u32 nextComboIndex          = animSet.attackComboIndex + 1;
                    desiredState                 = AttackStateFromIndex(nextComboIndex);
                } else {
                    // コンボをリセットする。desiredStateは上で計算した移動/ジャンプ判定のまま使う。
                    // 3段目が終わった直後は、先行入力が残っていても連撃を継続させず一度破棄する
                    // （3段目終了＝一区切りとして、必ず新しい入力から次のコンボを始めさせる）
                    animSet.attackComboIndex    = 0;
                    animSet.attackInputBuffered = false;
                }
            }

            bool willBeAttacking = IsAttackState(desiredState);

            //-------------------------------------------------------------
            // 攻撃中は移動させない（PlayerSystemが今フレーム書き込んだ
            // moveInputを、Physicsが消費する前にここで打ち消す）
            //-------------------------------------------------------------
            if(willBeAttacking) {
                cc.moveInput = hlslpp::float3(0.0f, 0.0f, 0.0f);
            }

            //-------------------------------------------------------------
            // ステートマシンに遷移を委譲する（変化がなければ何もしない。
            // 変化していればOnExit→OnEnterの順でコールバックが呼ばれ、クリップが切り替わる）
            //-------------------------------------------------------------
            m_stateMachine.TransitionTo(animSet.currentState, desiredState, registry, entity);

            //-------------------------------------------------------------
            // 装備中の武器へ「攻撃アニメーション再生中か」を伝える
            // （CombatSystemがこれを見て、浮遊演出のON/OFFと手ボーンへの追従度を切り替える）
            //-------------------------------------------------------------
            if(hasWeapon) {
                registry.GetComponent<WeaponComponent>(player.weaponEntity).isAttacking = willBeAttacking;
            }

#if defined(_DEBUG)
            //-------------------------------------------------------------
            // 連撃の分割点調整用ログ。0.1秒間隔に間引いて出す。
            // WeaponGripDebugSystemのF10（ポーズ固定）/F11（1コマ送り）と併用し、
            // フレーム番号を見ながらActionGameScene.cppの分割定数を追い込む
            //-------------------------------------------------------------
            if(willBeAttacking) {
                static float debugLogTimer = 0.0f;
                debugLogTimer              += deltaTime;
                if(debugLogTimer > 0.1f) {
                    debugLogTimer = 0.0f;

                    const auto& step       = animSet.attackSteps[animSet.attackComboIndex];
                    float       stepLength = std::max(step.endTime - step.startTime, 0.0001f);
                    // animPlayer.elapsed_timeはこの段のクリップレンジ先頭からの経過秒（playbackSpeed適用済み）。
                    // 段自身の経過を別途real timeで数えるより、実際に進んだクリップ位置を直接見る方が
                    // playbackSpeedを変えたときもズレない
                    float       normalized = animPlayer.elapsed_time / stepLength;
                    // ticksPerSecondを持たないためここでは30fps相当で概算する（実クリップのfpsとずれる場合がある点に注意）
                    float       clipFrame  = (step.startTime + animPlayer.elapsed_time) * 30.0f;

                    Tsukino::Core::Log::Info(
                        "ATTACK step=" + std::to_string(animSet.attackComboIndex + 1) + "/"
                        + std::to_string(PlayerAnimationSetComponent::kAttackComboCount)
                        + "  frame=" + std::to_string(clipFrame)
                        + " (clip " + std::to_string(step.startTime) + "-" + std::to_string(step.endTime) + "s)"
                        + "  speed=" + std::to_string(step.playbackSpeed)
                        + "  normalized=" + std::to_string(normalized)
                        + "  buffered=" + std::to_string(animSet.attackInputBuffered ? 1 : 0));
                }
            }
#endif
        });
    }
}    // namespace ActionGame::ECS

//-------------------------------------------------------------
//! @file   PlayerAnimationSystem.cpp
//! @brief  PlayerAnimationSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/ActionGame/ECS/System/PlayerAnimationSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerAnimationSetComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/CharacterControllerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp>

#include <Tsukino/Core/typedef.hpp>

#include <hlsl++.h>
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
                                                                        float                                                      fadeTime = kAnimBlendTime) {
            return [clipMember, animationIndex, looping, fadeTime](Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
                Tsukino::Asset::AssetHandle clip = registry.GetComponent<PlayerAnimationSetComponent>(entity).*clipMember;
                if(!clip.IsValid())
                    return;

                auto& animController = registry.GetComponent<Tsukino::BuiltIn::ECS::AnimationControllerComponent>(entity);

                animController.next.clip            = clip;
                animController.next.animation_index = animationIndex;
                animController.next.fade_time       = fadeTime;
                animController.next.immediate       = false;    // クロスフェードで切り替える
                animController.next.is_looping      = looping;
            };
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief コンストラクタ。各ステートのOnEnterコールバック（クリップ切り替え）を登録する
    //-------------------------------------------------------------
    PlayerAnimationSystem::PlayerAnimationSystem() {
        // これらのMixamo由来のFBXは、いずれもindex 0が「Armature」レイヤーの1tickのみのスタブ、
        // index 1が実際の全ボーンモーション（52チャンネル）になっているため、再生には1を指定する
        m_stateMachine.RegisterState(PlayerAnimState::Idle, MakeClipEnterCallback(&PlayerAnimationSetComponent::idleClip, 1, true));
        m_stateMachine.RegisterState(PlayerAnimState::Run, MakeClipEnterCallback(&PlayerAnimationSetComponent::runClip, 1, true));
        m_stateMachine.RegisterState(PlayerAnimState::FastRun, MakeClipEnterCallback(&PlayerAnimationSetComponent::fastRunClip, 1, true));
        m_stateMachine.RegisterState(PlayerAnimState::Jump, MakeClipEnterCallback(&PlayerAnimationSetComponent::jumpClip, 1, false));
    }

    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void PlayerAnimationSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto view = registry.View<PlayerComponent, Tsukino::BuiltIn::ECS::CharacterControllerComponent, PlayerAnimationSetComponent>();
        view.each([&](entt::entity                                          entity,
                     PlayerComponent&                                      player,
                     Tsukino::BuiltIn::ECS::CharacterControllerComponent& cc,
                     PlayerAnimationSetComponent&                          animSet) {
            //-------------------------------------------------------------
            // 現在のプレイヤーの状態から、あるべきアニメーションステートを決定する
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

            //-------------------------------------------------------------
            // ステートマシンに遷移を委譲する（変化がなければ何もしない。
            // 変化していればOnExit→OnEnterの順でコールバックが呼ばれ、クリップが切り替わる）
            //-------------------------------------------------------------
            m_stateMachine.TransitionTo(animSet.currentState, desiredState, registry, entity);
        });
    }
}    // namespace ActionGame::ECS

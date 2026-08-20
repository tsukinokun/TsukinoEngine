//-------------------------------------------------------------
//! @file   EnemyAnimationSystem.cpp
//! @brief  EnemyAnimationSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/ActionGame/ECS/System/EnemyAnimationSystem.hpp>

#include <Tsukino/BuiltIn/ECS/Component/AnimationControllerComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp>

#include <Tsukino/Core/typedef.hpp>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    namespace {
        constexpr float kAnimBlendTime   = 0.15f;    //!< Idle/Walkへ切り替える際のクロスフェード時間（秒）
        constexpr float kAttackBlendTime = 0.10f;    //!< Attackへ切り替える際のクロスフェード時間（秒。素早く反応させるため短め）

        //-------------------------------------------------------------
        //! @brief  「指定クリップへクロスフェードする」OnEnterコールバックを作るヘルパー
        //!         （PlayerAnimationSystem::MakeClipEnterCallbackと同じ考え方）
        //! @param  clipMember [in] EnemyAnimationSetComponentが持つクリップハンドルへのメンバポインタ
        //! @param  looping    [in] ループ再生するか
        //! @param  fadeTime   [in] クロスフェードにかける時間（秒）
        //! @param  inPlace    [in] ルートモーションの水平成分を殺すか
        //-------------------------------------------------------------
        StateMachine<EnemyAnimState>::Callback MakeClipEnterCallback(Tsukino::Asset::AssetHandle EnemyAnimationSetComponent::* clipMember,
                                                                        bool                                                     looping,
                                                                        float                                                    fadeTime,
                                                                        bool                                                     inPlace) {
            return [clipMember, looping, fadeTime, inPlace](Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
                auto&                        animSet = registry.GetComponent<EnemyAnimationSetComponent>(entity);
                Tsukino::Asset::AssetHandle clip     = animSet.*clipMember;
                if(!clip.IsValid())
                    return;

                auto& animController = registry.GetComponent<Tsukino::BuiltIn::ECS::AnimationControllerComponent>(entity);

                animController.next.clip            = clip;
                animController.next.animation_index = animSet.animationIndex;
                animController.next.fade_time       = fadeTime;
                animController.next.immediate       = false;    // クロスフェードで切り替える
                animController.next.is_looping      = looping;
                animController.next.clip_start_time = 0.0f;     // クリップ全体を再生する
                animController.next.clip_end_time   = 0.0f;
                animController.next.in_place        = inPlace;

                registry.GetComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(entity).playback_speed = 1.0f;
            };
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief コンストラクタ。各ステートのOnEnterコールバック（クリップ切り替え）を登録する
    //-------------------------------------------------------------
    EnemyAnimationSystem::EnemyAnimationSystem() {
        // 待機用クリップが無いため、Idle/Walkともに歩行クリップを使う。
        // Idleはin_place=trueでその場足踏みにし、移動を表すWalkと視覚的に区別する必要はない
        // （追跡の有無自体はTransformの移動で表現される）
        m_stateMachine.RegisterState(EnemyAnimState::Idle, MakeClipEnterCallback(&EnemyAnimationSetComponent::walkClip, true, kAnimBlendTime, true));
        m_stateMachine.RegisterState(EnemyAnimState::Walk, MakeClipEnterCallback(&EnemyAnimationSetComponent::walkClip, true, kAnimBlendTime, true));

        // Attackへ入るときはattackTimer（BigZombieBehavior::PlayAttackの終了判定ウォッチドッグ）も
        // ここでリセットする。PlayAttackはこのOnEnterが実行される1フレーム前から加算を始めているため、
        // 実際にAttackクリップへ切り替わった瞬間を基準に測り直す
        auto attackClipEnter = MakeClipEnterCallback(&EnemyAnimationSetComponent::attackClip, false, kAttackBlendTime, true);
        m_stateMachine.RegisterState(EnemyAnimState::Attack, [attackClipEnter](Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity) {
            attackClipEnter(registry, entity);
            registry.GetComponent<EnemyAnimationSetComponent>(entity).attackTimer = 0.0f;
        });
    }

    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void EnemyAnimationSystem::Update(Tsukino::ECS::Registry& registry, float /*deltaTime*/) {
        auto view = registry.View<EnemyAnimationSetComponent, Tsukino::BuiltIn::ECS::AnimationPlayerComponent>();
        view.each([&](entt::entity entity, EnemyAnimationSetComponent& animSet, Tsukino::BuiltIn::ECS::AnimationPlayerComponent&) {
            // EnemyBehaviorSystemが書いたdesiredStateへ委譲する。変化がなければ何もしない。
            // 変化していればOnExit→OnEnterの順でコールバックが呼ばれ、クリップが切り替わる
            m_stateMachine.TransitionTo(animSet.currentState, animSet.desiredState, registry, entity);
        });
    }
}    // namespace ActionGame::ECS

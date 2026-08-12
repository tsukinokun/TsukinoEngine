//-------------------------------------------------------------
//! @file   DiceThrowUtil.cpp
//! @brief  サイコロの投擲・リセット処理の共通関数の実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Util/DiceThrowUtil.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/DiceComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/RoundComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp>

#include <cstdlib>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {
    namespace {
        //-------------------------------------------------------------
        // 揺り直し（ThrowDiceSet）用の暫定値。DiceRestDetectionSystemの閾値と同様、
        // 実機で転がり方を見ながら調整すること。
        //-------------------------------------------------------------
        constexpr float kShakeUpwardImpulse     = 4.0f;    //!< 上方向への基本インパルス
        constexpr float kShakeHorizontalImpulse = 3.0f;    //!< 水平方向のランダムインパルスの振れ幅
        constexpr float kShakeAngularImpulse    = 6.0f;    //!< 回転（タンブリング）用のランダム角インパルスの振れ幅

        //-------------------------------------------------------------
        // 場外復帰（RepositionDiceAboveBowl）用の暫定値
        //-------------------------------------------------------------
        constexpr float kReturnHeight = 10.0f;    //!< 戻す高さ（お椀中心からの相対Y座標）
        constexpr float kReturnSpeed  = 40.0f;    //!< お椀中心方向へ戻す際の目標速度

        //-------------------------------------------------------------
        //! @brief  -1.0f 〜 1.0f のランダム値を返す
        //-------------------------------------------------------------
        float RandomUnit() {
            return (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
        }

        //-------------------------------------------------------------
        //! @brief  ImpulseRequestComponentを加算する形で設定する
        //!         （同一フレーム内で他のシステムから既に要求が積まれていても上書きしないため）
        //-------------------------------------------------------------
        void AccumulateImpulse(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const hlslpp::float3& impulse,
                                const hlslpp::float3& angularImpulse) {
            if(auto* existing = registry.try_get<Tsukino::BuiltIn::ECS::ImpulseRequestComponent>(entity)) {
                existing->impulse += impulse;
                existing->angularImpulse += angularImpulse;
            } else {
                Tsukino::BuiltIn::ECS::ImpulseRequestComponent& request =
                    registry.AddComponent<Tsukino::BuiltIn::ECS::ImpulseRequestComponent>(entity);
                request.impulse        = impulse;
                request.angularImpulse = angularImpulse;
            }
        }

        //-------------------------------------------------------------
        //! @brief  1つのサイコロの判定関連フラグをリセットする
        //-------------------------------------------------------------
        void ResetDiceJudgeState(DiceComponent& dice) {
            dice.confirmed      = false;
            dice.confirmedValue = 0;
            dice.settleTimer    = 0.0f;
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief  サイコロをお椀中心上空へ再配置する
    //-------------------------------------------------------------
    void RepositionDiceAboveBowl(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity diceEntity, const hlslpp::float3& bowlCenter) {
        auto& rigidbody = registry.GetComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(diceEntity);
        auto& transform = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(diceEntity);

        // Dynamicボディはトランスフォームへの直接書き込みだけでは戻せない（毎フレーム物理演算側の
        // 座標で上書きされる）ため、現在の速度を打ち消しつつお椀中心方向へ一定速度で押し戻す
        // インパルスを与えることで擬似的にテレポートさせる。
        hlslpp::float3 targetPosition = bowlCenter + hlslpp::float3(0.0f, kReturnHeight, 0.0f);
        hlslpp::float3 toTarget       = targetPosition - transform.position;
        float          distance       = hlslpp::length(toTarget).x;
        hlslpp::float3 direction      = (distance > 0.001f) ? (toTarget / distance) : hlslpp::float3(0.0f, 1.0f, 0.0f);

        hlslpp::float3 impulse        = rigidbody.mass * (direction * kReturnSpeed - rigidbody.linearVelocity);
        hlslpp::float3 angularImpulse = rigidbody.mass * (-rigidbody.angularVelocity);

        AccumulateImpulse(registry, diceEntity, impulse, angularImpulse);
    }

    //-------------------------------------------------------------
    //! @brief  RoundComponentが束ねる3つのサイコロを投げ直す
    //-------------------------------------------------------------
    void ThrowDiceSet(Tsukino::ECS::Registry& registry, RoundComponent& round) {
        for(Tsukino::ECS::Entity diceEntity : round.dice) {
            DiceComponent& dice = registry.GetComponent<DiceComponent>(diceEntity);

            dice.state = DiceRollState::Rolling;
            ResetDiceJudgeState(dice);

            // お椀を揺するイメージで、上方向＋ランダムな水平・回転インパルスを与える
            hlslpp::float3 impulse(RandomUnit() * kShakeHorizontalImpulse, kShakeUpwardImpulse, RandomUnit() * kShakeHorizontalImpulse);
            hlslpp::float3 angularImpulse(RandomUnit() * kShakeAngularImpulse, RandomUnit() * kShakeAngularImpulse, RandomUnit() * kShakeAngularImpulse);

            AccumulateImpulse(registry, diceEntity, impulse, angularImpulse);
        }

        round.judged   = false;
        round.kind     = Hand::None;
        round.subValue = 0;
    }

    //-------------------------------------------------------------
    //! @brief  RoundComponentが束ねる3つのサイコロを、投げ直さずに待機状態へ戻す
    //-------------------------------------------------------------
    void ResetRoundToIdle(Tsukino::ECS::Registry& registry, RoundComponent& round) {
        for(Tsukino::ECS::Entity diceEntity : round.dice) {
            DiceComponent& dice = registry.GetComponent<DiceComponent>(diceEntity);

            dice.state = DiceRollState::Idle;
            ResetDiceJudgeState(dice);
        }

        round.judged   = false;
        round.kind     = Hand::None;
        round.subValue = 0;
    }

}    // namespace LuckGameSampleScene::ECS

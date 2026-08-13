//-------------------------------------------------------------
//! @file   DiceThrowUtil.cpp
//! @brief  サイコロの投擲・リセット処理の共通関数の実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Util/DiceThrowUtil.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/DiceComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/RoundComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/RoundOwnerComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp>

#include <cstdlib>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {
    namespace {
        //-------------------------------------------------------------
        // 投下待ち（SetupDiceHover）用の暫定値
        //-------------------------------------------------------------
        // 強すぎると（特に投下待ちが長時間続いた場合に）投下時の角速度が過大になり、
        // 着地後にお椀の外まで弾き飛ばされて「暴れる」原因になるため、弱めに設定している
        constexpr float kHoverSpinTorque = 0.8f;    //!< 投下待ち中にその場で回転させ続けるトルクの強さ

        //-------------------------------------------------------------
        // 投下待ち位置（ComputeDiceSpawnOffset）用の暫定値。
        // お椀の実測AABBは概ね12×12×6ユニット（半径6程度）なので、3個ともお椀の中に
        // 収まるよう間隔を狭めにしている。実機で調整すること。
        //-------------------------------------------------------------
        constexpr float kDiceSpawnHeight   = 10.0f;    //!< 投下待ち高さ（お椀中心からの相対Y座標）
        constexpr float kDiceSpawnSpacingX = 2.5f;     //!< 3個を並べるX方向の間隔

        //-------------------------------------------------------------
        // 場外復帰（RepositionDiceAboveBowl）用の暫定値
        //-------------------------------------------------------------
        constexpr float kReturnHeight = kDiceSpawnHeight;    //!< 戻す高さ（お椀中心からの相対Y座標）
        constexpr float kReturnSpeed  = 40.0f;               //!< お椀中心方向へ戻す際の目標速度

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
    //! @brief  diceIndex(0〜2)番目のサイコロの投下待ち位置（お椀中心からの相対座標）を返す
    //-------------------------------------------------------------
    hlslpp::float3 ComputeDiceSpawnOffset(int diceIndex) {
        return hlslpp::float3(static_cast<float>(diceIndex - 1) * kDiceSpawnSpacingX, kDiceSpawnHeight, 0.0f);
    }

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
    //! @brief  1つのサイコロを「投下待ち（空中で静止＋回転）」状態にする
    //-------------------------------------------------------------
    void SetupDiceHover(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity diceEntity) {
        auto&          rigidbody = registry.GetComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(diceEntity);
        DiceComponent& dice      = registry.GetComponent<DiceComponent>(diceEntity);

        // 位置を全軸フリーズし、重力の影響を受けても空中に留まるようにする。
        // 回転は逆にフリーズを解除する（前回Settledでフリーズされたままだと、
        // 振り直し後にトルクを与えても回転せず「投下待ちで回転する」演出が動かなくなるため）
        rigidbody.freezePositionX = true;
        rigidbody.freezePositionY = true;
        rigidbody.freezePositionZ = true;
        rigidbody.freezeRotationX = false;
        rigidbody.freezeRotationY = false;
        rigidbody.freezeRotationZ = false;
        rigidbody.isFreezeDirty   = true;

        // 投下（DropDiceSet）されるまでその場で回転し続けるよう、ランダムな向きのトルクを与え続ける
        rigidbody.torque = hlslpp::float3(RandomUnit(), RandomUnit(), RandomUnit()) * kHoverSpinTorque;

        dice.state = DiceRollState::Hovering;
        ResetDiceJudgeState(dice);
    }

    //-------------------------------------------------------------
    //! @brief  RoundComponentが束ねる3つのサイコロを投下する
    //! @return 1個でも実際に投下できたらtrue（全てHovering以外だった場合はfalse）
    //-------------------------------------------------------------
    bool DropDiceSet(Tsukino::ECS::Registry& registry, RoundComponent& round) {
        bool anyDropped = false;

        for(Tsukino::ECS::Entity diceEntity : round.dice) {
            DiceComponent& dice = registry.GetComponent<DiceComponent>(diceEntity);

            // Hovering以外（既に投下済み・投げ直し待ちなど）は対象外
            // ＝この関数が誤って複数回呼ばれても、2回目以降は各サイコロにつき何もしない（多重投下の防止）
            if(dice.state != DiceRollState::Hovering) {
                continue;
            }

            auto& rigidbody = registry.GetComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(diceEntity);

            // 位置フリーズを解除し、重力とその時点の角速度に任せて落とす
            rigidbody.freezePositionX = false;
            rigidbody.freezePositionY = false;
            rigidbody.freezePositionZ = false;
            rigidbody.isFreezeDirty   = true;
            rigidbody.torque          = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 空中回転用のトルクは止める

            dice.state = DiceRollState::Rolling;
            ResetDiceJudgeState(dice);
            anyDropped = true;
        }

        if(anyDropped) {
            round.judged   = false;
            round.kind     = Hand::None;
            round.subValue = 0;
        }

        return anyDropped;
    }

    //-------------------------------------------------------------
    //! @brief  RoundComponentが束ねる3つのサイコロを投下待ち位置へリスポンさせる
    //-------------------------------------------------------------
    void RespawnDiceSet(Tsukino::ECS::Registry& registry, RoundComponent& round) {
        for(std::size_t i = 0; i < round.dice.size(); ++i) {
            Tsukino::ECS::Entity diceEntity = round.dice[i];

            auto&          rigidbody = registry.GetComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>(diceEntity);
            auto&          transform = registry.GetComponent<Tsukino::BuiltIn::ECS::TransformComponent>(diceEntity);
            RoundOwnerComponent& owner = registry.GetComponent<RoundOwnerComponent>(diceEntity);
            DiceComponent&       dice  = registry.GetComponent<DiceComponent>(diceEntity);

            // Dynamicのままではトランスフォームへの直接書き込みが次の物理フレームで上書きされてしまう
            // ため、一旦Kinematic化して「今のトランスフォームが正」の状態でテレポートさせる。
            // DiceRespawnSystemが1〜2フレック後にDynamic＋フリーズ（Hovering）へ引き継ぐ。
            rigidbody.type        = Tsukino::BuiltIn::ECS::RigidbodyType::Kinematic;
            rigidbody.isTypeDirty = true;

            transform.position = owner.bowlCenter + ComputeDiceSpawnOffset(static_cast<int>(i));
            transform.rotation = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
            transform.dirty    = true;

            dice.state = DiceRollState::Respawning;
            ResetDiceJudgeState(dice);
        }

        round.judged   = false;
        round.kind     = Hand::None;
        round.subValue = 0;
    }

}    // namespace LuckGameSampleScene::ECS

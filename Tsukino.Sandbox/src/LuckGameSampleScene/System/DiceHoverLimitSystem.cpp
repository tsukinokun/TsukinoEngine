//-------------------------------------------------------------
//! @file   DiceHoverLimitSystem.cpp
//! @brief  DiceHoverLimitSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/DiceHoverLimitSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/DiceComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ImpulseRequestComponent.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {
    namespace {
        //-------------------------------------------------------------
        // 投下待ち中に許容する角速度の上限（暫定値）。
        // SetupDiceHoverの継続トルクは待機時間が長いほど角速度を際限なく
        // 増やしてしまうため、ここで頭打ちにする。実機で「待ってから投げても
        // 短く投げたときと同じくらいの勢いになる」かを見ながら調整すること。
        //-------------------------------------------------------------
        constexpr float kMaxHoverAngularSpeed = 3.0f;
    }

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void DiceHoverLimitSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        (void)deltaTime;

        auto view = registry.View<DiceComponent, Tsukino::BuiltIn::ECS::RigidbodyComponent>();

        view.each([&](entt::entity entity, DiceComponent& dice, Tsukino::BuiltIn::ECS::RigidbodyComponent& rigidbody) {
            // 投下待ち（Hovering）中のサイコロのみが対象
            if(dice.state != DiceRollState::Hovering) {
                return;
            }

            float currentSpeed = hlslpp::length(rigidbody.angularVelocity).x;
            if(currentSpeed <= kMaxHoverAngularSpeed) {
                return;
            }

            // 上限を超えた分だけ打ち消す角力積を与え、角速度を上限まで頭打ちにする
            hlslpp::float3 targetAngularVelocity = rigidbody.angularVelocity * (kMaxHoverAngularSpeed / currentSpeed);
            hlslpp::float3 angularImpulse        = rigidbody.mass * (targetAngularVelocity - rigidbody.angularVelocity);

            if(auto* existing = registry.try_get<Tsukino::BuiltIn::ECS::ImpulseRequestComponent>(entity)) {
                existing->angularImpulse += angularImpulse;
            } else {
                Tsukino::BuiltIn::ECS::ImpulseRequestComponent& request =
                    registry.AddComponent<Tsukino::BuiltIn::ECS::ImpulseRequestComponent>(entity);
                request.impulse        = hlslpp::float3(0.0f, 0.0f, 0.0f);
                request.angularImpulse = angularImpulse;
            }
        });
    }

}    // namespace LuckGameSampleScene::ECS

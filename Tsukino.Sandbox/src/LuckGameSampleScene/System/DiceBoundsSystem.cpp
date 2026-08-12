//-------------------------------------------------------------
//! @file   DiceBoundsSystem.cpp
//! @brief  DiceBoundsSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/DiceBoundsSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/DiceComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/RoundOwnerComponent.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Util/DiceThrowUtil.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/Log.hpp>

#include <cmath>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {
    namespace {
        //-------------------------------------------------------------
        // 場外判定の半径（暫定値）。
        // お椀モデルの実測AABBは概ね半径6ユニット程度（実寸スケール）なので、
        // その2倍強を目安にしている。実機で調整すること。
        //-------------------------------------------------------------
        constexpr float kOutOfBoundsRadius = 14.0f;
    }

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void DiceBoundsSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        (void)deltaTime;

        auto view = registry.View<Tsukino::BuiltIn::ECS::TransformComponent, DiceComponent, RoundOwnerComponent>();

        view.each([&](entt::entity entity, Tsukino::BuiltIn::ECS::TransformComponent& transform, DiceComponent& dice, RoundOwnerComponent& owner) {
            // 投げられていないサイコロ、既に静止確定したサイコロは対象外
            if(dice.state != DiceRollState::Rolling) {
                return;
            }

            const float dx             = transform.position.x - owner.bowlCenter.x;
            const float dz             = transform.position.z - owner.bowlCenter.z;
            const float distanceFromXZ = std::sqrt(dx * dx + dz * dz);

            if(distanceFromXZ <= kOutOfBoundsRadius) {
                return;
            }

            // 場外に出たので、判定を進めず（rollCountも増やさず）お椀中心上空へ戻す
            RepositionDiceAboveBowl(registry, entity, owner.bowlCenter);
            dice.settleTimer = 0.0f;

#ifdef _DEBUG
            Tsukino::Core::Log::Info("[LuckGameSampleScene] Dice out of bounds, returning to bowl.");
#endif
        });
    }

}    // namespace LuckGameSampleScene::ECS

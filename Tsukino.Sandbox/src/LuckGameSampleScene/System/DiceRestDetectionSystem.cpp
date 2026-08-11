//-------------------------------------------------------------
//! @file   DiceRestDetectionSystem.cpp
//! @brief  DiceRestDetectionSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/DiceRestDetectionSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/DiceComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/Core/Log.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {
    namespace {
        //-------------------------------------------------------------
        // 静止判定のしきい値。
        // 実機で RigidbodyComponent.linearVelocity / angularVelocity の実測値を見ながら
        // フェーズ2の動作確認手順に従って調整すること（暫定値）。
        //-------------------------------------------------------------
        constexpr float kLinearVelocityThreshold  = 2.0f;     //!< この速さを下回ったら「遅い」とみなす
        constexpr float kAngularVelocityThreshold = 0.5f;     //!< この角速度を下回ったら「遅い」とみなす
        constexpr float kSettleSeconds            = 0.3f;     //!< この時間だけ「遅い」状態が継続したら静止確定
    }

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void DiceRestDetectionSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto view = registry.View<Tsukino::BuiltIn::ECS::RigidbodyComponent, DiceComponent>();

        view.each([&](entt::entity, Tsukino::BuiltIn::ECS::RigidbodyComponent& rigidbody, DiceComponent& dice) {
            // 投げられていない、あるいは既に静止確定済みのサイコロは無視する
            if(dice.state != DiceRollState::Rolling) {
                return;
            }

            const float linearSpeed  = hlslpp::length(rigidbody.linearVelocity).x;
            const float angularSpeed = hlslpp::length(rigidbody.angularVelocity).x;
            const bool  isSlowEnough = (linearSpeed < kLinearVelocityThreshold) && (angularSpeed < kAngularVelocityThreshold);

            if(isSlowEnough) {
                dice.settleTimer += deltaTime;
            } else {
                // バウンドなどで再び動き出したら積算をリセットする
                dice.settleTimer = 0.0f;
            }

            if(dice.settleTimer >= kSettleSeconds) {
                dice.state = DiceRollState::Settled;

#ifdef _DEBUG
                Tsukino::Core::Log::Info("[LuckGameSampleScene] Dice settled.");
#endif
            }
        });
    }

}    // namespace LuckGameSampleScene::ECS

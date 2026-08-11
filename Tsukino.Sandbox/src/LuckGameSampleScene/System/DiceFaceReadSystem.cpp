//-------------------------------------------------------------
//! @file   DiceFaceReadSystem.cpp
//! @brief  DiceFaceReadSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/DiceFaceReadSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/DiceComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Core/Log.hpp>
#include <string>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {
    namespace {
        // ワールド空間での重力方向（上向き判定の基準）
        const hlslpp::float3 kUpDirection = hlslpp::float3(0.0f, 1.0f, 0.0f);
    }

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void DiceFaceReadSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        (void)deltaTime;

        auto view = registry.View<Tsukino::BuiltIn::ECS::TransformComponent, DiceComponent>();

        view.each([&](entt::entity, Tsukino::BuiltIn::ECS::TransformComponent& transform, DiceComponent& dice) {
            // 静止確定していない、あるいは既に出目が確定済みなら何もしない
            if(dice.state != DiceRollState::Settled || dice.confirmed) {
                return;
            }

            float bestDot   = -1.0f;
            u8    bestValue = dice.faceValue[0];

            for(int i = 0; i < 6; ++i) {
                // ローカル法線をワールド回転で変換し、上向きベクトルとの内積を見る
                hlslpp::float3 worldNormal = hlslpp::mul(transform.rotation, dice.faceNormal[i]);
                float          dotValue    = hlslpp::dot(worldNormal, kUpDirection).x;

                if(dotValue > bestDot) {
                    bestDot   = dotValue;
                    bestValue = dice.faceValue[i];
                }
            }

            dice.confirmedValue = bestValue;
            dice.confirmed      = true;

#ifdef _DEBUG
            Tsukino::Core::Log::Info("[LuckGameSampleScene] Dice face confirmed: " + std::to_string(bestValue));
#endif
        });
    }

}    // namespace LuckGameSampleScene::ECS

//-------------------------------------------------------------
//! @file   CompareSystem.hpp
//! @brief  CompareSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @class  CompareSystem
    //! @brief  両プレイヤーの手番が確定(TurnPhase::Resolved)したら役の強さを比較し、
    //!         GameStateComponent::outcome / phase を確定させるシステム。
    //!         優先度は TurnRuleSystem より後に登録すること。
    //-------------------------------------------------------------
    class CompareSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace LuckGameSampleScene::ECS

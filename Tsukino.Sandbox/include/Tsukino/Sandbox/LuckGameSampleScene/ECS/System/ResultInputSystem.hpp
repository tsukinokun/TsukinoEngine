//-------------------------------------------------------------
//! @file   ResultInputSystem.hpp
//! @brief  ResultInputSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @class  ResultInputSystem
    //! @brief  GamePhase::Result中のスペース入力を検知し、
    //!         GameSceneManager::ChangeScene によってシーンを丸ごと読み直すシステム。
    //-------------------------------------------------------------
    class ResultInputSystem : public Tsukino::ECS::ISystem {
    public:
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };

}    // namespace LuckGameSampleScene::ECS

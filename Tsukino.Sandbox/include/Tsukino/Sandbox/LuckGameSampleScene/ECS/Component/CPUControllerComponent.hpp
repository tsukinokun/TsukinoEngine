//-------------------------------------------------------------
//! @file   CPUControllerComponent.hpp
//! @brief  CPUControllerComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @struct CPUControllerComponent
    //! @brief  このコンポーネントを持つPlayerエンティティはCPUとして自動制御される
    //-------------------------------------------------------------
    struct CPUControllerComponent {
        float rerollDelayTimer = 0.0f;    //!< 0より大きい間は「考え中」演出。0になったら振り直しを実行する
    };

}    // namespace LuckGameSampleScene::ECS

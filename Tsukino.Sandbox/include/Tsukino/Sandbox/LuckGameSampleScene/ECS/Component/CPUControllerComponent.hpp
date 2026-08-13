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
        float rerollDelayTimer = 0.0f;    //!< 0より大きい間は「考え中」演出。0になったら投下待ち(isDropPending)へ移行する
        bool  isDropPending    = false;   //!< 考え中タイマーは消化済みで、あとはサイコロがHoveringに
                                           //!< 達し次第（リスポンの完了を待って）投下する状態か
    };

}    // namespace LuckGameSampleScene::ECS

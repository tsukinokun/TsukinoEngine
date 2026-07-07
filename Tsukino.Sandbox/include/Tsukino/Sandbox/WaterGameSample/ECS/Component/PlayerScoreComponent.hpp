//-------------------------------------------------------------
//! @file   PlayerScoreComponent.hpp
//! @brief  プレイヤーのスコア保持コンポーネント
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    //-------------------------------------------------------------
    //! @struct PlayerScoreComponent
    //-------------------------------------------------------------
    struct PlayerScoreComponent {
        int score = 0;    //!< 現在のスコア（食べたドットのscoreValue合計）
    };

}    // namespace WaterGame::ECS

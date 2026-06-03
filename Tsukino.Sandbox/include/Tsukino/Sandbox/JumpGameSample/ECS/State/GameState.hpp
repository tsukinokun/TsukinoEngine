//-------------------------------------------------------------
//! @file   GameState.hpp
//! @brief  GameStateクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
// 名前空間 : JumpGameSample::ECS
namespace JumpGameSample::ECS {
    //-------------------------------------------------------------
    //! @enum class  GameState
    //-------------------------------------------------------------
    enum class GameState {
        Ready,      // 開始待ち
        Playing,    // プレイ中
        GameOver    // ゲームオーバー演出
    };
}    // namespace JumpGameSample::ECS

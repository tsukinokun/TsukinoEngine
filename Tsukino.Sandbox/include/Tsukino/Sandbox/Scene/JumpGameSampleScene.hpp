//-------------------------------------------------------------
//! @file    JumpGameSampleScene.hpp
//! @brief   ジャンプゲームのサンプルシーンの宣言
//! @author  山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    //-------------------------------------------------------------
    //! @class   JumpGameSampleScene
    //! @brief   ジャンプゲームのサンプルシーン
    //-------------------------------------------------------------
    class JumpGameSampleScene : public Tsukino::EngineIntegration::GameSceneBase {
    public:
        // ゲームの状態定義
        enum class GameState {
            Ready,      // 開始待ち
            Playing,    // プレイ中
            GameOver    // ゲームオーバー演出
        };

        JumpGameSampleScene()           = default;
        ~JumpGameSampleScene() override = default;

        //-------------------------------------------------------------
        //! @brief  シーンの更新
        //! @param  api       [in] エンジンから提供されるAPIへの参照
        //! @param  deltaTime [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        void OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) override;

        //-------------------------------------------------------------
        //! @brief  シーンの終了処理
        //-------------------------------------------------------------
        void OnExit() override;

    protected:
        //-------------------------------------------------------------
        //! @brief  シーン固有の初期化処理
        //! @param  api [in] エンジンから提供されるAPIへの参照
        //-------------------------------------------------------------
        void OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) override;

        GameState m_currentState = GameState::Ready;
        bool      isGameOver     = false;    // ゲームオーバー状態のフラグ
    };
}    // namespace Tsukino::Sandbox

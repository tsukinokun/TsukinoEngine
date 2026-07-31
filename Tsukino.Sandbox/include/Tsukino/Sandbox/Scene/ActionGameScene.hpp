//-------------------------------------------------------------
//! @file    ActionGameScene.hpp
//! @brief   アクションゲームのサンプルシーンの宣言
//! @author  山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    //-------------------------------------------------------------
    //! @class   ActionGameScene
    //! @brief   アクションのサンプルシーン
    //-------------------------------------------------------------
    class ActionGameScene : public Tsukino::EngineIntegration::GameSceneBase {
    public:
        //-------------------------------------------------------------
        //! @brief  コンストラクタ
        //-------------------------------------------------------------
        ActionGameScene() = default;

        //-------------------------------------------------------------
        //! @brief  デストラクタ
        //-------------------------------------------------------------
        ~ActionGameScene() override = default;

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

    private:
        //-------------------------------------------------------------
        //! @brief  シーン固有の初期化処理
        //! @param  api [in] エンジンから提供されるAPIへの参照
        //-------------------------------------------------------------
        void OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) override;

    private:
        //-------------------------------------------------------------
        //! @enum class GameState
        //-------------------------------------------------------------
        enum class GameState {
            Playing,    //!< プレイ中
            TimeUp      //!< 時間切れ。スペースキー待機中
        };

        GameState m_gameState = GameState::Playing;    //!< ゲームの状態
    };
}    // namespace Tsukino::Sandbox

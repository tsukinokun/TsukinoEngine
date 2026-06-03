//-------------------------------------------------------------
//! @file    JumpGameSampleScene.hpp
//! @brief   ジャンプゲームのサンプルシーンの宣言
//! @author  山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Sandbox/JumpGameSample/ECS/State/GameState.hpp>

#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    //-------------------------------------------------------------
    //! @class   JumpGameSampleScene
    //! @brief   ジャンプゲームのサンプルシーン
    //-------------------------------------------------------------
    class JumpGameSampleScene : public Tsukino::EngineIntegration::GameSceneBase {
    public:
        //-------------------------------------------------------------
        //! @brief  コンストラクタ
        //-------------------------------------------------------------
        JumpGameSampleScene() = default;

        //-------------------------------------------------------------
        //! @brief  デストラクタ
        //-------------------------------------------------------------
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

    private:
        //-------------------------------------------------------------
        //! @brief  シーン固有の初期化処理
        //! @param  api [in] エンジンから提供されるAPIへの参照
        //-------------------------------------------------------------
        void OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) override;

        JumpGameSample::ECS::GameState mCurrentState = JumpGameSample::ECS::GameState::Ready;
        bool                      isGameOver    = false;    // ゲームオーバー状態のフラグ
    };
}    // namespace Tsukino::Sandbox

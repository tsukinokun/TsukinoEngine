//-------------------------------------------------------------
//! @file    PenguinGameScene.hpp
//! @brief   ペンギンゲームのサンプルシーンの宣言
//! @author  山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Sandbox/JumpGameSample/ECS/State/GameState.hpp>

#include <Tsukino/Sandbox/PenguinGame/Data/PenguinSkinDefinition.hpp>

#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    //-------------------------------------------------------------
    //! @class   PenguinGameScene
    //! @brief   ペンギンゲームのサンプルシーン
    //-------------------------------------------------------------
    class PenguinGameScene : public Tsukino::EngineIntegration::GameSceneBase {
    public:
        //-------------------------------------------------------------
        //! @brief  コンストラクタ
        //-------------------------------------------------------------
        PenguinGameScene() = default;

        //-------------------------------------------------------------
        //! @brief  デストラクタ
        //-------------------------------------------------------------
        ~PenguinGameScene() override = default;

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

        // 現在使用中のペンギンスキンの定義
        PenguinGame::Data::PenguinSkinDefinition m_Skin;   
    };
}    // namespace Tsukino::Sandbox

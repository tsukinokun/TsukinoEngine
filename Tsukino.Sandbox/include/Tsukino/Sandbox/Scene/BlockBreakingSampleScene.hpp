//-------------------------------------------------------------
//! @file    BlockBreakingSampleScene.hpp
//! @brief   ブロック崩しのサンプルシーンの宣言
//! @author  山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>
// 名前空間 : Tsukino::Sandbox
namespace Tsukino::Sandbox {
    //-------------------------------------------------------------
    //! @class   BlockBreakingSampleScene
    //! @brief   ブロック崩しのサンプルシーン
    //-------------------------------------------------------------
    class BlockBreakingSampleScene : public Tsukino::EngineIntegration::GameSceneBase {
    public:
        BlockBreakingSampleScene()           = default;
        ~BlockBreakingSampleScene() override = default;

        //-------------------------------------------------------------
        //! @brief  シーンの更新
        //-------------------------------------------------------------
        void OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) override;

        //-------------------------------------------------------------
        //! @brief  シーンの終了処理
        //-------------------------------------------------------------
        void OnExit() override;

    protected:
        //-------------------------------------------------------------
        //! @brief  シーン固有の初期化処理
        //-------------------------------------------------------------
        void OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) override;
    };
}    // namespace Tsukino::Sandbox

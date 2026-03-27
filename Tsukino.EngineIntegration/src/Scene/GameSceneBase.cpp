//-------------------------------------------------------------
//! @file    GameSceneBase.cpp
//! @brief   ゲームシーンの基底クラスの宣言
//! @author  山﨑愛
//-------------------------------------------------------------
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>
namespace Tsukino::EngineIntegration {
    //-------------------------------------------------------------
    //! @brief シーンの初期化インターフェース（外部から呼ばれる非仮想関数）
    //-------------------------------------------------------------
    void GameSceneBase::Initialize(Tsukino::EngineIntegration::EngineAPI& api, Tsukino::EngineIntegration::EngineContext* context) {
        //-------------------------------------------------------------
        // コンテキストを設定する
        //-------------------------------------------------------------
        m_scene.GetRegistry().SetContext<Tsukino::EngineIntegration::EngineContext*>(context);

        //-------------------------------------------------------------
        // 各シーン固有の初期化処理を呼び出す
        //-------------------------------------------------------------
        OnInitialize(api);
    }
}    // namespace Tsukino::EngineIntegration

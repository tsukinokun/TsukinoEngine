//-------------------------------------------------------------
//! @file    GameSceneBase.hpp
//! @brief   ゲームシーンの基底クラスの宣言
//! @author  山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/ECS/Scene.hpp>
// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {
    // 前方宣言
    class EngineAPI;
    struct EngineContext;
    //-------------------------------------------------------------
    //! @class   GameSceneBase
    //! @brief   ゲームシーンの基底クラス
    //-------------------------------------------------------------
    class GameSceneBase {
    public:
        //-------------------------------------------------------------
        //! @brief  デフォルトコンストラクタ
        //-------------------------------------------------------------
        virtual ~GameSceneBase() = default;

        //-------------------------------------------------------------
        // シーンの初期化インターフェース（外部から呼ばれる非仮想関数）
        //! @param  api     [in] エンジンから提供されるAPIへの参照
        //! @param  context [in] エンジンコンテキストへのポインタ
        //-------------------------------------------------------------
        void Initialize(Tsukino::EngineIntegration::EngineAPI& api, Tsukino::EngineIntegration::EngineContext* context);

        //-------------------------------------------------------------
        //! @brief  シーンの更新インターフェース
        //! @param  api       [in] エンジンから提供されるAPIへの参照
        //! @param  deltaTime [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        virtual void OnUpdate(Tsukino::EngineIntegration::EngineAPI& api, float deltaTime) = 0;

        //-------------------------------------------------------------
        //! @brief  シーンの終了インターフェース
        //-------------------------------------------------------------
        virtual void OnExit() = 0;

        //-------------------------------------------------------------
        //! @brief  シーンへのアクセス
        //! @return シーンへの参照
        //-------------------------------------------------------------
        Tsukino::ECS::Scene& GetScene() { return m_scene; }

    protected:
        //-------------------------------------------------------------
        //! @brief  【派生クラスで実装】シーン固有の初期化処理
        //! @param  api [in] エンジンから提供されるAPIへの参照
        //-------------------------------------------------------------
        virtual void OnInitialize(Tsukino::EngineIntegration::EngineAPI& api) = 0;

    protected:
        Tsukino::ECS::Scene m_scene;    // シーンのインスタンス
    };
}    // namespace Tsukino::EngineIntegration

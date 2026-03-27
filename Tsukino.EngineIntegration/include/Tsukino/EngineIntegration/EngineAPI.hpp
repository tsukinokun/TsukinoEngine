//------------------------------------------------------------
//! @file    EngineAPI.hpp
//! @brief   エンジンからAPIを提供するクラスの宣言
//! @author  山﨑愛
//------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>

#include <memory>    

// 前方宣言
namespace Tsukino {
    namespace ECS {
        class ISystem;
    }
}

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {
    struct EngineContext;    // 前方宣言

    //------------------------------------------------------------
    //! @class   EngineAPI
    //! @brief   エンジンからAPIを提供するクラス
    //------------------------------------------------------------
    class EngineAPI {
    public:
        //------------------------------------------------------------
        // コンストラクタ
        //! @param   context エンジン全体で共有されるクラスのポインタを集めた構造体への参照
        //------------------------------------------------------------
        explicit EngineAPI(EngineContext& context);

        //------------------------------------------------------------
        // シーン遷移関数
        //! @param  newScene [in] 次に遷移するシーンのインスタンス
        //------------------------------------------------------------
        void ChangeScene(std::unique_ptr<GameSceneBase> newScene);

        //------------------------------------------------------------
        // メッセージ処理関数
        //! @return メッセージ処理が成功した場合は true、ウィンドウが閉じられた場合は false
        //------------------------------------------------------------
        [[nodiscard]]
        bool ProcessMessages();

        //------------------------------------------------------------
        // 更新関数
        //------------------------------------------------------------
        void Update(float deltaTime);

        //------------------------------------------------------------
        // 描画関数
        //------------------------------------------------------------
        void Render();

    private:
        EngineContext& m_context;    // コンテキストへの参照
    };

}    // namespace Tsukino::EngineIntegration

//------------------------------------------------------------
//! @file    EngineAPI.hpp
//! @brief   エンジンからAPIを提供するクラスの宣言
//! @author  山﨑愛
//------------------------------------------------------------
#pragma once
#include <Tsukino/Tsukino.EngineIntegration/EngineContext.hpp>

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

    private:
        EngineContext& m_context;    // コンテキストへの参照
    };

}    // namespace Tsukino::EngineIntegration

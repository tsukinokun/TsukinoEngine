//------------------------------------------------------------
//! @file       EngineAPI.cpp
//! @brief      エンジンからAPIを提供するクラスの実装
//! @author     山﨑愛
//------------------------------------------------------------
#include <Tsukino/Tsukino.EngineIntegration/EngineAPI.hpp>

#include <memory>

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {
    //------------------------------------------------------------
    //! @brief コンストラクタ
    //------------------------------------------------------------
    EngineAPI::EngineAPI(EngineContext& context)
        : m_context(context) {
    }
}    // namespace Tsukino::EngineIntegration

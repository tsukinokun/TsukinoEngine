//------------------------------------------------------------
//! @file       EngineIntegration.cpp
//! @brief      エンジン全体で共有されるクラスを統合したクラスの実装
//! @author     山﨑愛
//------------------------------------------------------------
#include <Tsukino/Tsukino.EngineIntegration/EngineIntegration.hpp>

#include <memory>

// 名前空間 : Tsukino::Integration
namespace Tsukino::EngineIntegration {
    //------------------------------------------------------------
    //! @brief コンストラクタ
    //------------------------------------------------------------
    EngineIntegration::EngineIntegration() {
        //------------------------------------------------------------
        // ユニークポインタを作成して
        //------------------------------------------------------------
        m_window       = std::make_unique<Tsukino::Core::Window>();
        m_renderer     = std::make_unique<Tsukino::Renderer::Renderer>();
        m_assetManager = std::make_unique<Tsukino::Asset::AssetManager>();

        //------------------------------------------------------------
        // コンテキストにポインタをセット
        //------------------------------------------------------------
        m_ctx.window   = m_window.get();
        m_ctx.renderer = m_renderer.get();
        m_ctx.assets   = m_assetManager.get();
    }

}    // namespace Tsukino::EngineIntegration

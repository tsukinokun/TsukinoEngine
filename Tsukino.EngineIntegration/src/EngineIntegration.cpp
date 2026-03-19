//------------------------------------------------------------
//! @file       EngineIntegration.cpp
//! @brief      エンジン全体で共有されるクラスを統合したクラスの実装
//! @author     山﨑愛
//------------------------------------------------------------
#include <Tsukino/Tsukino.EngineIntegration/EngineIntegration.hpp>

#include <Tsukino/Core/Log.hpp>

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

    //------------------------------------------------------------
    //! @brief  エンジンの初期化関数
    //------------------------------------------------------------
    bool EngineIntegration::Initialize() {
        //--------------------------------------------------------------
        // COM初期化
        //--------------------------------------------------------------
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        // COMの初期化に失敗した場合はエラーログを出力して終了
        if(!hr) {
            Tsukino::Core::Log::Error("Failed to initialize COM library.");
            return -1;
        }

        //--------------------------------------------------------------
        // AssetManager 初期化
        //--------------------------------------------------------------
        Tsukino::Asset::AssetManager::Initialize();

        //--------------------------------------------------------------
        // ウィンドウ生成
        //--------------------------------------------------------------
        Tsukino::Core::Window window;
        if(!window.Create("TsukinoEngine", 1280, 720)) {
            return -1;
        }

        //--------------------------------------------------------------
        // レンダラー生成
        //--------------------------------------------------------------
        Tsukino::Renderer::Renderer renderer;
        if(!renderer.Initialize(window.GetHWND(), window.GetWidth(), window.GetHeight())) {
            return -1;
        }

        return true;
    }
}    // namespace Tsukino::EngineIntegration

//------------------------------------------------------------
//! @file       EngineIntegration.cpp
//! @brief      エンジン全体で共有されるクラスを統合したクラスの実装
//! @author     山﨑愛
//------------------------------------------------------------
#include <Tsukino/EngineIntegration/EngineIntegration.hpp>

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
        m_window           = std::make_unique<Tsukino::Core::Window>();
        m_inputSystem      = std::make_unique<Tsukino::Input::InputSystem>();
        m_renderer         = std::make_unique<Tsukino::Renderer::Renderer>();
        m_assetManager     = std::make_unique<Tsukino::Asset::AssetManager>();
        m_builtinAssets    = std::make_unique<Tsukino::BuiltIn::BuiltInAssets>();
        m_gameSceneManager = std::make_unique<GameSceneManager>();
        m_audioManager     = std::make_unique<Tsukino::Audio::AudioManager>();

        //------------------------------------------------------------
        // コンテキストにポインタをセット
        //------------------------------------------------------------
        m_ctx.window           = m_window.get();
        m_ctx.renderer         = m_renderer.get();
        m_ctx.assetManager     = m_assetManager.get();
        m_ctx.builtinAssets    = m_builtinAssets.get();
        m_ctx.gameSceneManager = m_gameSceneManager.get();
        m_ctx.inputSystem      = m_inputSystem.get();
        m_ctx.audioManager     = m_audioManager.get();
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
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to initialize COM library.");
            return false;
        }

        //--------------------------------------------------------------
        // AssetManager 初期化
        //--------------------------------------------------------------
        m_assetManager->Initialize();

        //--------------------------------------------------------------
        // ビルトインアセットの初期化
        //--------------------------------------------------------------
        m_builtinAssets->Initialize(m_assetManager.get());

        //--------------------------------------------------------------
        // コンテキストを渡してGameSceneManagerを初期化
        //--------------------------------------------------------------
        m_gameSceneManager->Initialize(&m_ctx);

        //--------------------------------------------------------------
        // ウィンドウ生成
        //--------------------------------------------------------------
        if(!m_window->Create("TsukinoEngine", 1280, 720)) {
            return false;
        }

        //--------------------------------------------------------------
        // オーディオマネージャの初期化
        //--------------------------------------------------------------
        if (!m_audioManager->Initialize()) {
            Tsukino::Core::Log::Error("Failed to initialize AudioManager.");
            return false;
        }

        //--------------------------------------------------------------
        // メッセージコールバックの設定
        //--------------------------------------------------------------
        m_window->SetMessageCallback([this](UINT msg, WPARAM wp, LPARAM lp) {
            switch(msg) {
            // --- キーボード入力 ---
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:    // Altキーなどを含める場合
                m_inputSystem->SetKeyState(static_cast<Input::KeyCode>(wp), true);
                break;

            case WM_KEYUP:
            case WM_SYSKEYUP:
                m_inputSystem->SetKeyState(static_cast<Input::KeyCode>(wp), false);
                break;

            // --- マウス座標 ---
            case WM_MOUSEMOVE:
                // LOWORD, HIWORD で X, Y 座標を取り出す
                m_inputSystem->SetMousePosition(static_cast<i32>(LOWORD(lp)), static_cast<i32>(HIWORD(lp)));
                break;

            // --- マウスホイール ---
            case WM_MOUSEWHEEL:
                // ホイールの回転量を取り出して加算（120単位で届くので割る）
                float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wp)) / static_cast<float>(WHEEL_DELTA);
                m_inputSystem->AddWheelDelta(delta);
                break;
            }
        });

        //--------------------------------------------------------------
        // レンダラー生成
        //--------------------------------------------------------------
        if(!m_renderer->Initialize(m_window->GetHWND(), m_window->GetWidth(), m_window->GetHeight())) {
            return false;
        }

        return true;
    }
}    // namespace Tsukino::EngineIntegration

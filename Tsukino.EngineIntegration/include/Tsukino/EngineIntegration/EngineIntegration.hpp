//------------------------------------------------------------
//! @file       EngineIntegration.hpp
//! @brief      エンジン全体で共有されるクラスを統合したクラスの定義
//! @author     山﨑愛
//------------------------------------------------------------
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneManager.hpp>

#include <Tsukino/BuiltIn/BuiltInAssets.hpp>

#include <Tsukino/Renderer/Renderer.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp>

#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Window.hpp>
#include <Tsukino/Audio/AudioManager.hpp>

#include <memory>

// 名前空間 : Tsukino::Integration
namespace Tsukino::EngineIntegration {
    //------------------------------------------------------------
    //! @class   EngineIntegration
    //! @brief   エンジン全体で共有されるクラスを統合したクラス
    //------------------------------------------------------------
    class EngineIntegration {
    public:
        //------------------------------------------------------------
        // コンストラクタ
        //------------------------------------------------------------
        EngineIntegration();

        //------------------------------------------------------------
        //! @brief デフォルトデストラクタ
        //------------------------------------------------------------
        ~EngineIntegration() = default;

        //------------------------------------------------------------
        // エンジンの初期化関数
        //! @param width  [in] ウィンドウ幅
        //! @param height [in] ウィンドウ高さ
        //! @return true: 初期化成功, false: 初期化失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool Initialize(int width, int height, Tsukino::Core::Window::WindowStyle style = Tsukino::Core::Window::WindowStyle::Default);

        //------------------------------------------------------------
        //! @brief  エンジン全体で共有されるクラスを取得する関数
        //! @return クラスのポインタを集めた構造体
        //------------------------------------------------------------
        [[nodiscard]]
        EngineContext& GetContext() {
            return m_ctx;
        }

    private:
        //------------------------------------------------------------
        //! @brief  エンジン標準の組み込みコンポーネントを工場に自動登録する
        //------------------------------------------------------------
        void RegisterBuiltInComponents();

    private:
        std::unique_ptr<Tsukino::Renderer::Renderer>                 m_renderer;
        std::unique_ptr<Tsukino::Asset::AssetManager>                m_assetManager;
        std::unique_ptr<Tsukino::Core::Window>                       m_window;
        std::unique_ptr<Tsukino::BuiltIn::BuiltInAssets>             m_builtinAssets;
        std::unique_ptr<GameSceneManager>                            m_gameSceneManager;
        std::unique_ptr<Tsukino::Input::InputSystem>                 m_inputSystem;
        std::unique_ptr<Tsukino::Audio::AudioManager>                m_audioManager;
        std::unique_ptr<Tsukino::Engine::ECS::Prefab::PrefabFactory> m_prefabFactory;

        EngineContext m_ctx;    // エンジン全体で共有されるクラスのポインタを集めた構造体
    };
}    // namespace Tsukino::EngineIntegration

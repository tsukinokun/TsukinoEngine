//------------------------------------------------------------
//! @file       EngineIntegration.hpp
//! @brief      エンジン全体で共有されるクラスを統合したクラスの定義
//! @author     山﨑愛
//------------------------------------------------------------
#pragma once
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
#include <string>

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
        //! @brief デストラクタ
        //! @note  Initialize() で呼んだ CoInitializeEx と対にするため、
        //!        全メンバの破棄後に CoUninitialize() を呼ぶ必要がある。
        //------------------------------------------------------------
        ~EngineIntegration();

        //------------------------------------------------------------
        // COM とウィンドウを握るためコピー・ムーブ禁止
        //------------------------------------------------------------
        EngineIntegration(const EngineIntegration&)            = delete;
        EngineIntegration& operator=(const EngineIntegration&) = delete;
        EngineIntegration(EngineIntegration&&)                 = delete;
        EngineIntegration& operator=(EngineIntegration&&)      = delete;

        //------------------------------------------------------------
        // エンジンの初期化関数
        //! @param width  [in] ウィンドウ幅
        //! @param height [in] ウィンドウ高さ
        //! @param title  [in] ウィンドウタイトル
        //! @return true: 初期化成功, false: 初期化失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool Initialize(int width, int height, const std::string& title = "TsukinoEngine",
                         Tsukino::Core::Window::WindowStyle style = Tsukino::Core::Window::WindowStyle::Default);

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
        //------------------------------------------------------------
        //! @note 【この宣言順は破棄順序の設計であり、並べ替えてはならない】
        //!
        //! メンバは宣言の逆順に破棄されるため、上に書いたものほど長生きする。
        //!
        //! GameSceneManager のデストラクタは現在のシーンの OnExit() を呼び、
        //! シーンが持つ System は破棄時に Renderer / AssetManager /
        //! AudioManager / Window を参照しうる。
        //! したがって GameSceneManager は「最初に破棄される」必要があり、
        //! （m_ctx を除いて）最後に宣言する。
        //!
        //! 以前は m_gameSceneManager が m_audioManager より前に宣言されており、
        //! シーンの後始末より先に AudioManager が消える順序になっていた。
        //------------------------------------------------------------
        std::unique_ptr<Tsukino::Core::Window>                       m_window;
        std::unique_ptr<Tsukino::Renderer::Renderer>                 m_renderer;
        std::unique_ptr<Tsukino::Asset::AssetManager>                m_assetManager;
        std::unique_ptr<Tsukino::BuiltIn::BuiltInAssets>             m_builtinAssets;
        std::unique_ptr<Tsukino::Input::InputSystem>                 m_inputSystem;
        std::unique_ptr<Tsukino::Audio::AudioManager>                m_audioManager;
        std::unique_ptr<Tsukino::Engine::ECS::Prefab::PrefabFactory> m_prefabFactory;
        std::unique_ptr<GameSceneManager>                            m_gameSceneManager;    // 最初に破棄する

        EngineContext m_ctx;    // エンジン全体で共有されるクラスのポインタを集めた構造体

        bool m_comInitialized = false;    // CoInitializeEx に成功したか（対になる CoUninitialize の要否判定用）
    };
}    // namespace Tsukino::EngineIntegration

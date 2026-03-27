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

#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <Tsukino/Core/Window.hpp>

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
        //! @return true: 初期化成功, false: 初期化失敗
        //------------------------------------------------------------
        [[nodiscard]]
        bool Initialize();

        //------------------------------------------------------------
        //! @brief  エンジン全体で共有されるクラスを初期化する関数
        //! @return クラスのポインタを集めた構造体
        //------------------------------------------------------------
        [[nodiscard]]
        EngineContext& GetContext() {
            return m_ctx;
        }

    private:
        std::unique_ptr<Tsukino::Renderer::Renderer>     m_renderer;
        std::unique_ptr<Tsukino::Asset::AssetManager>    m_assetManager;
        std::unique_ptr<Tsukino::Core::Window>           m_window;
        std::unique_ptr<Tsukino::BuiltIn::BuiltInAssets> m_builtinAssets;
        std::unique_ptr<GameSceneManager>                m_gameSceneManager;

        EngineContext m_ctx;    // エンジン全体で共有されるクラスのポインタを集めた構造体
    };
}    // namespace Tsukino::EngineIntegration

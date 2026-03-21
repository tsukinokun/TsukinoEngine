//------------------------------------------------------------
//! @file       EngineIntegration.hpp
//! @brief      エンジン全体で共有されるクラスを統合したクラスの定義
//! @author     山﨑愛
//------------------------------------------------------------
#include <Tsukino/EngineIntegration/EngineContext.hpp>

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

        //------------------------------------------------------------
        //! @brief  ECSレジストリへの参照を返す関数
        //! @return ECSレジストリへの参照
        //------------------------------------------------------------
        [[nodiscard]]
        Tsukino::ECS::Registry& GetRegistry() {
            return m_registry;
        }

    private:
        std::unique_ptr<Tsukino::Renderer::Renderer>     m_renderer;
        std::unique_ptr<Tsukino::Asset::AssetManager>    m_assetManager;
        std::unique_ptr<Tsukino::Core::Window>           m_window;
        std::unique_ptr<Tsukino::BuiltIn::BuiltInAssets> m_builtinAssets;

        EngineContext m_ctx;    // エンジン全体で共有されるクラスのポインタを集めた構造体

        Tsukino::ECS::Registry m_registry;
    };
}    // namespace Tsukino::EngineIntegration

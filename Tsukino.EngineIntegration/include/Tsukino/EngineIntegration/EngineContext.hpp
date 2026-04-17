//------------------------------------------------------------
//! @file   EngineContext.hpp
//! @brief  エンジン全体で共有されるクラスのポインタ集
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
// 名前空間 : Tsukino
namespace Tsukino {
    namespace Renderer {
        class Renderer;
    }

    namespace Core {
        class Window;
    }

    namespace Input {
        class InputSystem;
    }

    namespace Asset {
        class AssetManager;
    }

    namespace BuiltIn {
        class BuiltInAssets;
    }

    namespace Audio {
        class AudioManager;
    }
}    // namespace Tsukino

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {
    class GameSceneManager;    // 前方宣言
    //------------------------------------------------------------
    //! @struct  EngineContext
    //! @brief   エンジン全体で共有されるクラスのポインタを集めた構造体
    //------------------------------------------------------------
    struct EngineContext {
        Tsukino::Renderer::Renderer*     renderer         = nullptr;
        Tsukino::Core::Window*           window           = nullptr;
        Tsukino::Input::InputSystem*     inputSystem      = nullptr;
        Tsukino::Asset::AssetManager*    assetManager           = nullptr;
        Tsukino::BuiltIn::BuiltInAssets* builtinAssets    = nullptr;
        GameSceneManager*                gameSceneManager = nullptr;
        Tsukino::Audio::AudioManager*    audioManager     = nullptr;
    };
}    // namespace Tsukino::EngineIntegration

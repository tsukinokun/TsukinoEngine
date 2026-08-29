//------------------------------------------------------------
//! @file   EngineContext.hpp
//! @brief  エンジン全体で共有されるクラスのポインタ集
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
// 名前空間 : Tsukino
namespace Tsukino {
    namespace BuiltIn::ECS {
        class EffectSystem;
        class PhysicsSystem;
    }
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

    namespace Engine::ECS::Prefab {
        class PrefabFactory;
    }
}    // namespace Tsukino

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {
    class GameSceneManager;    // 前方宣言
    //------------------------------------------------------------
    //! @struct  EngineContext
    //! @brief   エンジン全体で共有されるクラスのポインタと、少量の共有stateを集めた構造体
    //------------------------------------------------------------
    struct EngineContext {
        Tsukino::Renderer::Renderer*                 renderer         = nullptr;
        Tsukino::Core::Window*                       window           = nullptr;
        Tsukino::Input::InputSystem*                 inputSystem      = nullptr;
        Tsukino::Asset::AssetManager*                assetManager     = nullptr;
        Tsukino::BuiltIn::BuiltInAssets*             builtinAssets    = nullptr;
        GameSceneManager*                            gameSceneManager = nullptr;
        Tsukino::Audio::AudioManager*                audioManager     = nullptr;
        Tsukino::Engine::ECS::Prefab::PrefabFactory* prefabFactory    = nullptr;
        Tsukino::BuiltIn::ECS::EffectSystem*         effectSystem     = nullptr;
        Tsukino::BuiltIn::ECS::PhysicsSystem*        physicsSystem    = nullptr;
    };
}    // namespace Tsukino::EngineIntegration

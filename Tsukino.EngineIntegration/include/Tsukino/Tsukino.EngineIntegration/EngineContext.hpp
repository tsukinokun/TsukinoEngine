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
    namespace Asset {
        class AssetManager;
    }
}    // namespace Tsukino

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {
    //------------------------------------------------------------
    //! @struct  EngineContext
    //! @brief   エンジン全体で共有されるクラスのポインタを集めた構造体
    //------------------------------------------------------------
    struct EngineContext {
        Tsukino::Renderer::Renderer*  renderer = nullptr;
        Tsukino::Core::Window*        window   = nullptr;
        Tsukino::Asset::AssetManager* assets   = nullptr;
    };
}    // namespace Tsukino::EngineIntegration

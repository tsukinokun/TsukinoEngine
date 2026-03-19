//------------------------------------------------------------
//! @file   EngineContext.hpp
//! @brief  エンジン全体で共有されるクラスのポインタ集
//! @author 山﨑愛
//------------------------------------------------------------
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Core/Window.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
//------------------------------------------------------------
//! @struct  EngineContext
//! @brief   エンジン全体で共有されるクラスのポインタを集めた構造体
//------------------------------------------------------------
struct EngineContext {
    Tsukino::Renderer::Renderer* renderer = nullptr;
    Tsukino::Core::Window*       window   = nullptr;
    Tsukino::Asset::AssetManager* assets = nullptr;
};

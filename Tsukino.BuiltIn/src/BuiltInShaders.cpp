//--------------------------------------------------------------
//! @file   BuiltInShaders.cpp
//! @brief  ビルトインシェーダー集約クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/BuiltIn/BuiltInShaders.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <Tsukino/Core/Path.hpp>
// 名前空間 : Tsukino::BuiltIn
namespace Tsukino::BuiltIn {
    //--------------------------------------------------------------
    //! @brief ビルトインシェーダーの初期化関数
    //--------------------------------------------------------------
    void BuiltInShaders::Initialize(Tsukino::Asset::AssetManager* assetManager) {
        spriteVS       = assetManager->Load(Tsukino::Core::Path("Tsukino.BuiltIn/Assets/Shaders/Sprite.vs.hlsl"));
        spritePS       = assetManager->Load(Tsukino::Core::Path("Tsukino.BuiltIn/Assets/Shaders/Sprite.ps.hlsl"));
        modelVS        = assetManager->Load(Tsukino::Core::Path("Tsukino.BuiltIn/Assets/Shaders/Model.vs.hlsl"));
        modelPS        = assetManager->Load(Tsukino::Core::Path("Tsukino.BuiltIn/Assets/Shaders/Model.ps.hlsl"));
        staticModelVS  = assetManager->Load(Tsukino::Core::Path("Tsukino.BuiltIn/Assets/Shaders/ModelStatic.vs.hlsl"));
        debugVS        = assetManager->Load(Tsukino::Core::Path("Tsukino.BuiltIn/Assets/Shaders/Debug.vs.hlsl"));
        debugPS        = assetManager->Load(Tsukino::Core::Path("Tsukino.BuiltIn/Assets/Shaders/Debug.ps.hlsl"));
        shadowVS       = assetManager->Load(Tsukino::Core::Path("Tsukino.BuiltIn/Assets/Shaders/ShadowMap.vs.hlsl"));
        shadowStaticVS = assetManager->Load(Tsukino::Core::Path("Tsukino.BuiltIn/Assets/Shaders/ShadowMapStatic.vs.hlsl"));
        shadowPS       = assetManager->Load(Tsukino::Core::Path("Tsukino.BuiltIn/Assets/Shaders/ShadowMap.ps.hlsl"));
    }
}    // namespace Tsukino::BuiltIn

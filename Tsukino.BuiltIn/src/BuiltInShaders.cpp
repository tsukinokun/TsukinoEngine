//--------------------------------------------------------------
//! @file   BuiltInShaders.cpp
//! @brief  ビルトインシェーダー集約クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/BuiltIn/BuiltInShaders.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>
// 名前空間 : Tsukino::BuiltIn
namespace Tsukino::BuiltIn {
    //--------------------------------------------------------------
    //! @brief ビルトインシェーダーの初期化関数
    //--------------------------------------------------------------
    void BuiltInShaders::Initialize(Tsukino::Asset::AssetManager* assetManager) {
        // エンジン自身が所有するアセットのため、取り込み側リポジトリの
        // GetAssetRootPath()ではなくGetEngineAssetRootPath()から解決する
        const Tsukino::Core::Path root = Tsukino::IO::FileSystem::GetEngineAssetRootPath();

        spriteVS       = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/Sprite.vs.hlsl");
        spritePS       = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/Sprite.ps.hlsl");
        spriteWorldVS  = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/Sprite3D.vs.hlsl");
        modelVS        = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/Model.vs.hlsl");
        modelPS        = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/Model.ps.hlsl");
        staticModelVS  = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/ModelStatic.vs.hlsl");
        debugVS        = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/DebugLine.vs.hlsl");
        debugPS        = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/DebugLine.ps.hlsl");
        shadowVS       = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/ShadowMap.vs.hlsl");
        shadowStaticVS = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/ShadowMapStatic.vs.hlsl");
        shadowPS       = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/ShadowMap.ps.hlsl");
        skyVS          = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/Sky.vs.hlsl");
        skyPS          = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/Sky.ps.hlsl");
        tonemapVS      = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/Tonemap.vs.hlsl");
        tonemapPS      = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/Tonemap.ps.hlsl");
        waterPS        = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/Water.ps.hlsl");
        gbufferPS      = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/GBuffer.ps.hlsl");
        lightingPS     = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/Lighting.ps.hlsl");
        motionBlurPS   = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/MotionBlur.ps.hlsl");
        fogPS          = assetManager->Load(root / "Tsukino.BuiltIn/Assets/Shaders/Fog.ps.hlsl");
    }
}    // namespace Tsukino::BuiltIn

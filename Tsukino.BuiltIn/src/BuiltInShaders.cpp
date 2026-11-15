//--------------------------------------------------------------
//! @file   BuiltInShaders.cpp
//! @brief  ビルトインシェーダー集約クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/BuiltIn/BuiltInShaders.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>

#include <string>

// 名前空間 : Tsukino::BuiltIn
namespace Tsukino::BuiltIn {
    namespace {
        //--------------------------------------------------------------
        //! 組み込みのシェーダーを1本読み込みます。
        //! @param  [in] assetManager 読み込み先
        //! @param  [in] root         エンジンのアセットのルート
        //! @param  [in] name         シェーダーの名前（"Sprite.vs" のように、拡張子を除いたファイル名）
        //! @return シェーダーアセットのハンドル
        //! @note   Shaders/bgfx/<name>.sc を読む。ゲームのシェーダーと同じ経路で、
        //!         ビルド時に変換した結果（Shaders/bgfx/<形式>/<name>.sc.bin）が読まれる
        //--------------------------------------------------------------
        Tsukino::Asset::AssetHandle LoadBuiltInShader(Tsukino::Asset::AssetManager& assetManager, const Tsukino::Core::Path& root,
                                                      const std::string& name) {
            const Tsukino::Core::Path path = root / ("Tsukino.BuiltIn/Assets/Shaders/bgfx/" + name + ".sc");
            const Tsukino::Asset::AssetHandle handle = assetManager.Load(path);
            if(!handle.IsValid())
                Tsukino::Core::Log::Error("BuiltInShaders: Failed to load " + path.string());
            return handle;
        }
    }    // namespace

    //--------------------------------------------------------------
    //! @brief ビルトインシェーダーの初期化関数
    //--------------------------------------------------------------
    void BuiltInShaders::Initialize(Tsukino::Asset::AssetManager* assetManager) {
        // エンジン自身が所有するアセットのため、取り込み側リポジトリの
        // GetAssetRootPath()ではなくGetEngineAssetRootPath()から解決する
        const Tsukino::Core::Path root = Tsukino::IO::FileSystem::GetEngineAssetRootPath();

        spriteVS       = LoadBuiltInShader(*assetManager, root, "Sprite.vs");
        spritePS       = LoadBuiltInShader(*assetManager, root, "Sprite.ps");
        spriteWorldVS  = LoadBuiltInShader(*assetManager, root, "Sprite3D.vs");
        modelVS        = LoadBuiltInShader(*assetManager, root, "Model.vs");
        modelPS        = LoadBuiltInShader(*assetManager, root, "Model.ps");
        screenModelPS  = LoadBuiltInShader(*assetManager, root, "ScreenModel.ps");
        staticModelVS  = LoadBuiltInShader(*assetManager, root, "ModelStatic.vs");
        debugVS        = LoadBuiltInShader(*assetManager, root, "DebugLine.vs");
        debugPS        = LoadBuiltInShader(*assetManager, root, "DebugLine.ps");
        shadowVS       = LoadBuiltInShader(*assetManager, root, "ShadowMap.vs");
        shadowStaticVS = LoadBuiltInShader(*assetManager, root, "ShadowMapStatic.vs");
        shadowPS       = LoadBuiltInShader(*assetManager, root, "ShadowMap.ps");
        skyVS          = LoadBuiltInShader(*assetManager, root, "Sky.vs");
        skyPS          = LoadBuiltInShader(*assetManager, root, "Sky.ps");
        tonemapVS      = LoadBuiltInShader(*assetManager, root, "Tonemap.vs");
        tonemapPS      = LoadBuiltInShader(*assetManager, root, "Tonemap.ps");
        gbufferPS      = LoadBuiltInShader(*assetManager, root, "GBuffer.ps");
        lightingPS     = LoadBuiltInShader(*assetManager, root, "Lighting.ps");
        motionBlurPS   = LoadBuiltInShader(*assetManager, root, "MotionBlur.ps");
        fogPS          = LoadBuiltInShader(*assetManager, root, "Fog.ps");
        sphereColorGradePS = LoadBuiltInShader(*assetManager, root, "SphereColorGrade.ps");

        bloomPrefilterPS  = LoadBuiltInShader(*assetManager, root, "BloomPrefilter.ps");
        bloomDownsamplePS = LoadBuiltInShader(*assetManager, root, "BloomDownsample.ps");
        bloomUpsamplePS   = LoadBuiltInShader(*assetManager, root, "BloomUpsample.ps");

        ambientParticleVS = LoadBuiltInShader(*assetManager, root, "AmbientParticle.vs");
        ambientParticlePS = LoadBuiltInShader(*assetManager, root, "AmbientParticle.ps");

        iblIrradiancePS        = LoadBuiltInShader(*assetManager, root, "IrradianceConvolve.ps");
        iblSpecularPrefilterPS = LoadBuiltInShader(*assetManager, root, "SpecularPrefilter.ps");
        iblBRDFLUTPS            = LoadBuiltInShader(*assetManager, root, "BRDFIntegrate.ps");
    }
}    // namespace Tsukino::BuiltIn

//--------------------------------------------------------------
//! @file   BuiltInShaders.cpp
//! @brief  ビルトインシェーダー集約クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/BuiltIn/BuiltInShaders.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderLoader.hpp>
#include <Tsukino/Engine/Asset/Util/AssetHandleGenerator.hpp>

#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>

#include <string>

// 名前空間 : Tsukino::BuiltIn
namespace Tsukino::BuiltIn {
    namespace {
#if defined(TSUKINO_RENDERER_BGFX)
        //! bgfx 版で読む、変換済みのシェーダーの形式のフォルダ（Tools/Shaders/CMakeLists.txt の出力先）。
        //! 今は Windows の Direct3D 11（s_5_0）だけ
        constexpr const char* kBgfxShaderProfileDir = "dxbc";
#endif

        //--------------------------------------------------------------
        //! 組み込みのシェーダーを1本読み込みます。
        //! @param  [in] assetManager 読み込み先
        //! @param  [in] root         エンジンのアセットのルート
        //! @param  [in] name         シェーダーの名前（"Sprite.vs" のように、拡張子を除いたファイル名）
        //! @return シェーダーアセットのハンドル
        //! @note   DX11 版は Shaders/<name>.hlsl を読む（実行時にコンパイルする）。
        //!         bgfx 版は、ビルド時に変換した Shaders/bgfx/<形式>/<name>.sc.bin を読む。
        //!         まだ bgfx 版へ移していないシェーダーは .bin が無いので、HLSL 版を読んでおく
        //!         （bgfx の PipelineFactory は bgfx の形式でないシェーダーを受け付けず、その描画は出ない）
        //--------------------------------------------------------------
        Tsukino::Asset::AssetHandle LoadBuiltInShader(Tsukino::Asset::AssetManager& assetManager, const Tsukino::Core::Path& root,
                                                      const std::string& name) {
#if defined(TSUKINO_RENDERER_BGFX)
            const Tsukino::Core::Path binaryPath =
                root / ("Tsukino.BuiltIn/Assets/Shaders/bgfx/" + std::string(kBgfxShaderProfileDir) + "/" + name + ".sc.bin");
            if(Tsukino::IO::FileSystem::Exists(binaryPath)) {
                // 変換済みなので、インポート（キャッシュ）を通さずにそのまま読む
                Tsukino::Asset::ShaderLoader         loader;
                Tsukino::Core::Ref<Tsukino::Asset::IAsset> asset = loader.Load(binaryPath);
                if(asset) {
                    const Tsukino::Asset::AssetHandle handle = Tsukino::Asset::AssetHandleGenerator::GenerateFromKey(binaryPath.string());
                    std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(asset)->SetHandle(handle);
                    assetManager.RegisterAsset(handle, asset);
                    return handle;
                }
                Tsukino::Core::Log::Error("BuiltInShaders: Failed to read " + binaryPath.string());
            }
#endif
            return assetManager.Load(root / ("Tsukino.BuiltIn/Assets/Shaders/" + name + ".hlsl"));
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

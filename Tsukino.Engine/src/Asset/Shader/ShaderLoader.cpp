//--------------------------------------------------------------
//! @file   ShaderLoader.cpp
//! @brief  シェーダアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Shader/ShaderLoader.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>

#include <unordered_map>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する関数
    //--------------------------------------------------------------
    bool ShaderLoader::CanLoad(const std::string& ext) const {
        // 変換済みのシェーダー（<名前>.vs.sc.bin）。拡張子は最後の1つしか渡されない
        return ext == ".bin";
    }

    //--------------------------------------------------------------
    //! @brief シェーダファイルを読み込みShaderAssetを生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> ShaderLoader::Load(const Tsukino::Core::Path& path) {
        std::string filePath = path.string();

        // バイナリファイルを開く
        std::vector<u8> binary = Tsukino::IO::FileSystem::ReadBinary(path);
        if(binary.empty()) {
            return nullptr;
        }

        // アセット生成
        Tsukino::Core::Ref<ShaderAsset> asset = Tsukino::Core::CreateRef<ShaderAsset>();
        asset->filePath                       = filePath;

        // ステージ判定
        asset->shaderStage = DetectStage(path);

        // バイナリ読み込み
        asset->binary = std::move(binary);

        // アセットを返す
        return asset;
    }

    //--------------------------------------------------------------
    //! @brief  シェーダーステージをファイルパスから推測する関数
    //--------------------------------------------------------------
    Tsukino::Shader::ShaderStage ShaderLoader::DetectStage(const Tsukino::Core::Path& path) {
        using Stage = Tsukino::Shader::ShaderStage;

        static const std::unordered_map<std::string, Stage> table = {
            // tsukino_shaders() が .sc から作る
            {".vs.sc.bin", Stage::Vertex},
            {".ps.sc.bin", Stage::Pixel },
        };

        std::string name = path.filename();

        // ファイル名の末尾が特定のサフィックスで終わるかをチェックしてステージを判定
        for(const auto& [suffix, stage] : table) {
            if(name.ends_with(suffix)) {
                return stage;
            }
        }

        // どのサフィックスにもマッチしない場合は Unknown を返す
        return Stage::Unknown;
    }

    //--------------------------------------------------------------
    //! bgfx のシェーダーのソース（.sc）から、ビルド時に変換した結果のパスを返します。
    //--------------------------------------------------------------
    Tsukino::Core::Path ShaderLoader::ToBinaryPath(const Tsukino::Core::Path& sourcePath) {
        return sourcePath.parent_path() / std::string(GetBinaryDirectoryName()) / (sourcePath.filename() + ".bin");
    }

    //--------------------------------------------------------------
    //! この環境で読む、変換済みのシェーダーの形式のフォルダ名を返します。
    //--------------------------------------------------------------
    const char* ShaderLoader::GetBinaryDirectoryName() {
        // 今の bgfx 版は Windows の Direct3D 11（shaderc の s_5_0）だけ。
        // 他の OS（Metal・SPIR-V・GLES）は、その OS へ移すときに足す
        return "dxbc";
    }

}    // namespace Tsukino::Asset

//--------------------------------------------------------------
//! @file   FontImporter.cpp
//! @brief  フォントのインポータークラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Font/FontImporter.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>

#include <filesystem>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief  フォントのインポート関数
    //--------------------------------------------------------------
    bool FontImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        //--------------------------------------------------------------
        // ルートからの絶対パスで開く
        //--------------------------------------------------------------
        Tsukino::Core::Path baseDir           = Tsukino::IO::FileSystem::GetAssetRootPath();
        Tsukino::Core::Path absoluteInputPath = baseDir / inputPath;

        //--------------------------------------------------------------
        // outputDirectory(Cache/) に inputPath(相対) を結合して階層を維持
        //--------------------------------------------------------------
        Tsukino::Core::Path outputPath = outputDirectory / inputPath;
        outputPath.replace_extension(".spritefont");

        //--------------------------------------------------------------
        // 親ディレクトリの生成
        //--------------------------------------------------------------
        Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path());

        //--------------------------------------------------------------
        // .spritefontをキャッシュへコピー（既存は上書き）
        //--------------------------------------------------------------
        std::error_code ec;
        const bool copied = std::filesystem::copy_file(absoluteInputPath.string(), outputPath.string(), std::filesystem::copy_options::overwrite_existing, ec);

        if(!copied || ec) {
            Tsukino::Core::Log::Error("Failed to cache spritefont: " + absoluteInputPath.string() + " -> " + outputPath.string());
            return false;
        }

        Tsukino::Core::Log::Info("Font imported: " + outputPath.string());

        // インポート成功
        return true;
    }

}    // namespace Tsukino::Asset

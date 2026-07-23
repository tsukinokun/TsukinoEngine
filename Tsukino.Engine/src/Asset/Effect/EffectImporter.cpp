//--------------------------------------------------------------
//! @file   EffectImporter.cpp
//! @brief  エフェクトインポーターの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Effect/EffectImporter.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>

#include <fstream>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief ソースの .efk を出力先ディレクトリにコピーする
    //--------------------------------------------------------------
    bool EffectImporter::Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) {
        Tsukino::Core::Path baseDir           = Tsukino::IO::FileSystem::GetAssetRootPath();
        Tsukino::Core::Path absoluteInputPath = baseDir / inPutPath;

        Tsukino::Core::Path outputPath = outPutDirectory / inPutPath;

        Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path());

        std::ifstream src(absoluteInputPath.string(), std::ios::binary);
        if(!src.is_open()) {
            Tsukino::Core::Log::Error("EffectImporter: failed to open source: " + absoluteInputPath.string());
            return false;
        }

        std::ofstream dst(outputPath.string(), std::ios::binary);
        if(!dst.is_open()) {
            Tsukino::Core::Log::Error("EffectImporter: failed to open destination: " + outputPath.string());
            return false;
        }

        dst << src.rdbuf();

        return true;
    }

}    // namespace Tsukino::Asset

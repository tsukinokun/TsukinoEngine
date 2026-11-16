//--------------------------------------------------------------
//! @file   EffectImporter.cpp
//! @brief  エフェクトインポーターの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Effect/EffectImporter.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>

#include <cstdint>
#include <vector>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief ソースの .efk を出力先ディレクトリにコピーする
    //--------------------------------------------------------------
    bool EffectImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        Tsukino::Core::Path baseDir           = Tsukino::IO::FileSystem::GetAssetRootPath();
        Tsukino::Core::Path absoluteInputPath = baseDir / inputPath;

        Tsukino::Core::Path outputPath = outputDirectory / inputPath;

        // 出力先を作れないまま書き込みへ進むと、失敗が原因から遠い場所で
        // 「キャッシュが無い」として現れるため、ここで止める
        if(!Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path())) {
            Tsukino::Core::Log::Error("EffectImporter: Failed to create the output directory: " + outputPath.parent_path().string());
            return false;
        }

        const std::vector<std::uint8_t> data = Tsukino::IO::FileSystem::ReadBinary(absoluteInputPath);
        if(data.empty()) {
            Tsukino::Core::Log::Error("EffectImporter: failed to open source: " + absoluteInputPath.string());
            return false;
        }

        if(!Tsukino::IO::FileSystem::WriteBinary(outputPath, data.data(), data.size())) {
            Tsukino::Core::Log::Error("EffectImporter: failed to open destination: " + outputPath.string());
            return false;
        }

        return true;
    }

}    // namespace Tsukino::Asset

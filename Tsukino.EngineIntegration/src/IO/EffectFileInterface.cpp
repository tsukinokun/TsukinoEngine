#include <Tsukino/EngineIntegration/IO/EffectFileInterface.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Effekseer.h>
#include <filesystem>

namespace Tsukino::EngineIntegration {

void EffectFileInterface::SetBaseDirectory(const Tsukino::Core::Path& baseDir) {
    m_baseDirectory = baseDir;
}

std::vector<uint8_t> EffectFileInterface::ReadFile(const std::string& path) {
    return Tsukino::IO::FileSystem::ReadBinary(Tsukino::Core::Path(path));
}

bool EffectFileInterface::FileExists(const std::string& path) {
    return Tsukino::IO::FileSystem::Exists(Tsukino::Core::Path(path));
}

Effekseer::FileReaderRef EffectFileInterface::OpenRead(const EFK_CHAR* path) {
    if(!path) {
        Tsukino::Core::Log::Error("[EffectFileInterface::OpenRead] path is null");
        return nullptr;
    }

    auto              utf16Len = std::char_traits<char16_t>::length(path);
    std::vector<char> path8(utf16Len * 4 + 1, 0);
    Effekseer::ConvertUtf16ToUtf8(path8.data(), static_cast<int32_t>(path8.size()), path);

    std::string pathStr = path8.data();
    std::string filename = std::filesystem::path(pathStr).filename().string();

    Tsukino::Core::Log::Info("[EffectFileInterface::OpenRead] Current working directory: " + std::filesystem::current_path().string());
    Tsukino::Core::Log::Info("[EffectFileInterface::OpenRead] path: " + pathStr + ", filename: " + filename);

    std::vector<std::string> searchPaths;

    if(!m_baseDirectory.string().empty()) {
        searchPaths.push_back(m_baseDirectory.string() + "/" + pathStr);
        searchPaths.push_back(m_baseDirectory.string() + "/" + filename);
    }

    searchPaths.push_back("Assets/Effects/Texture/" + filename);
    searchPaths.push_back("Assets/Texture/" + filename);
    searchPaths.push_back("Texture/" + filename);
    searchPaths.push_back("Assets/" + pathStr);
    searchPaths.push_back(pathStr);

    for (const auto& searchPath : searchPaths) {
        if(FileExists(searchPath)) {
            Tsukino::Core::Log::Info("[EffectFileInterface::OpenRead] Found file at: " + searchPath);
            auto data = ReadFile(searchPath);
            Tsukino::Core::Log::Info("[EffectFileInterface::OpenRead] Read " + std::to_string(data.size()) + " bytes");
            return Effekseer::MakeRefPtr<EffectFileReader>(data);
        }
    }

    Tsukino::Core::Log::Error("[EffectFileInterface::OpenRead] File NOT found in any path for: " + pathStr);
    return nullptr;
}

Effekseer::FileWriterRef EffectFileInterface::OpenWrite(const EFK_CHAR* path) {
    (void)path;
    return nullptr;
}

} // namespace Tsukino::EngineIntegration

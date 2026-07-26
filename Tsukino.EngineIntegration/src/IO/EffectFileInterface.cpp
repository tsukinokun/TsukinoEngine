#include <Tsukino/EngineIntegration/IO/EffectFileInterface.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Effekseer.h>
#include <filesystem>

namespace Tsukino::EngineIntegration {

std::vector<uint8_t> EffectFileInterface::ReadFile(const std::string& path) {
    return Tsukino::IO::FileSystem::ReadBinary(Tsukino::Core::Path(path));
}

bool EffectFileInterface::FileExists(const std::string& path) {
    return Tsukino::IO::FileSystem::Exists(Tsukino::Core::Path(path));
}

static bool TextureFileExists(const std::string& filename) {
    std::vector<std::string> paths = {
        "Assets/Effects/Texture/" + filename,
        "Assets/Texture/" + filename,
        "Texture/" + filename,
        filename
    };
    
    Tsukino::Core::Log::Info("[TextureFileExists] Checking for: " + filename);
    
    for (const auto& path : paths) {
        bool exists = Tsukino::IO::FileSystem::Exists(Tsukino::Core::Path(path));
        Tsukino::Core::Log::Info("[TextureFileExists] Checking: " + path + " -> " + std::string(exists ? "EXISTS" : "NOT FOUND"));
        if (exists) {
            Tsukino::Core::Log::Info("[EffectFileInterface] Texture found at: " + path);
            return true;
        }
    }
    
    Tsukino::Core::Log::Error("[EffectFileInterface] Texture NOT found: " + filename);
    return false;
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
    
    TextureFileExists(filename);
    
    std::vector<std::string> searchPaths = {
        "Assets/Effects/Texture/" + filename,
        "Assets/Texture/" + filename,
        "Texture/" + filename,
        "Assets/" + pathStr,
        pathStr
    };
    
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

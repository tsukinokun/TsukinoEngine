//--------------------------------------------------------------
//! @file   AudioLoader.cpp
//! @brief  オーディオアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Audio/AudioLoader.hpp>
#include <Tsukino/Engine/Asset/Audio/AudioAsset.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {

    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する関数
    //--------------------------------------------------------------
    bool AudioLoader::CanLoad(const std::string& ext) const {
        const std::string lower = Tsukino::Core::Path::ToLower(ext);
        return lower == ".wav" || lower == ".mp3" || lower == ".flac";
    }

    //--------------------------------------------------------------
    //! @brief オーディオファイルを読み込み AudioAsset を生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> AudioLoader::Load(const Tsukino::Core::Path& path) {
        // 中身は再生するときに AudioManager が読む。ここでは在るかだけを確かめる
        if(!Tsukino::IO::FileSystem::Exists(path)) {
            Tsukino::Core::Log::Error("AudioLoader: File not found: " + path.string());
            return nullptr;
        }

        Tsukino::Core::Ref<AudioAsset> asset = Tsukino::Core::CreateRef<AudioAsset>();
        asset->filePath                      = path.string();
        return asset;
    }

}    // namespace Tsukino::Asset

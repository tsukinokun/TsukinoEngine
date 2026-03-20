//--------------------------------------------------------------
//! @file   TextureImporter.cpp
//! @brief  テクスチャのインポータークラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Texture/TextureImporter.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>

#include <DirectXTex/DirectXTex.h>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief  テクスチャのインポート関数
    //--------------------------------------------------------------
    bool TextureImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        DirectX::ScratchImage image;
        DirectX::TexMetadata  metadata;

        HRESULT hr = DirectX::LoadFromWICFile(inputPath.ToWString().c_str(), DirectX::WIC_FLAGS_NONE, &metadata, image);

        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to load texture: " + inputPath.string());
            return false;
        }

        // 出力DDSパス
        auto                name       = inputPath.stem();
        Tsukino::Core::Path outputPath = outputDirectory / (name + ".dds");

        // DDSファイルとして保存
        hr = DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(), metadata, DirectX::DDS_FLAGS_NONE, outputPath.ToWString().c_str());

        // 保存に失敗した場合はエラーログを出力して false を返す
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to save DDS: " + outputPath.string());
            return false;
        }

        // 成功した場合は情報ログを出力して true を返す
        Tsukino::Core::Log::Info("Texture imported: " + outputPath.string());

        return true;
    }

}    // namespace Tsukino::Asset

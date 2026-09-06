//--------------------------------------------------------------
//! @file   TextureImporter.cpp
//! @brief  テクスチャのインポータークラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Texture/TextureImporter.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include "TextureAlphaUtility.hpp"

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>

#include <DirectXTex/DirectXTex.h>

#include <cstdint>
#include <cstring>
#include <vector>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief  テクスチャのインポート関数
    //--------------------------------------------------------------
    bool TextureImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        //--------------------------------------------------------------
        // ルートからの絶対パスで開く
        //--------------------------------------------------------------
        Tsukino::Core::Path baseDir           = Tsukino::IO::FileSystem::GetAssetRootPath();
        Tsukino::Core::Path absoluteInputPath = baseDir / inputPath;

        //--------------------------------------------------------------
        // テクスチャをDirectXTexで読み込むための変数
        //--------------------------------------------------------------
        DirectX::ScratchImage image;
        DirectX::TexMetadata  metadata;

        HRESULT hr = DirectX::LoadFromWICFile(absoluteInputPath.ToWString().c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, &metadata, image);

        //--------------------------------------------------------------
        // metadata.format を sRGB に変換（ここが最重要）
        //--------------------------------------------------------------
        if(metadata.format == DXGI_FORMAT_R8G8B8A8_UNORM) {
            metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        }

        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to load texture: " + absoluteInputPath.string());
            return false;
        }

        //--------------------------------------------------------------
        // 透明ピクセルのRGBを近傍色で塗り広げる（バイリニア補間時の黒縁対策）
        //--------------------------------------------------------------
        Tsukino::Asset::Detail::DilateTransparentEdges(image);

        //--------------------------------------------------------------
        // outputDirectory(Cache/) に inputPath(相対) を結合して階層を維持
        // (inputPathが絶対パスの場合はToEngineRelativePath()で相対パスに戻してから結合する)
        //--------------------------------------------------------------
        Tsukino::Core::Path outputPath = outputDirectory / Tsukino::IO::FileSystem::ToEngineRelativePath(inputPath);
        outputPath.replace_extension(".dds");    // 拡張子をDDSに変更

        //--------------------------------------------------------------
        // 親ディレクトリの生成
        //--------------------------------------------------------------
        // 出力先を作れないまま書き込みへ進むと、失敗が原因から遠い場所で
        // 「キャッシュが無い」として現れるため、ここで止める
        if(!Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path())) {
            Tsukino::Core::Log::Error("TextureImporter: Failed to create the output directory: "
                                      + outputPath.parent_path().string());
            return false;
        }

        //--------------------------------------------------------------
        // DDSファイルとして保存
        //--------------------------------------------------------------
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

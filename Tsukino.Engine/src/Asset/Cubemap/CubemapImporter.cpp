//--------------------------------------------------------------
//! @file   CubemapImporter.cpp
//! @brief  キューブマップインポーターの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#define NOMINMAX
#include <Tsukino/Engine/Asset/Cubemap/CubemapImporter.hpp>
#include <Tsukino/Engine/Asset/Cubemap/CubemapDesc.hpp>

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <DirectXTex/DirectXTex.h>
#include <wincodec.h>

#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <fstream>
#include <array>
#include <string>

namespace Tsukino::Asset {

    bool CubemapImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        //--------------------------------------------------------------
        // 拡張子チェック
        //--------------------------------------------------------------
        std::string ext = inputPath.extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if(ext != ".cubemap") {
            Tsukino::Core::Log::Error("CubemapImporter: Unsupported format: " + ext);
            return false;
        }

        //--------------------------------------------------------------
        // .cubemapファイル（JSON）を読み込む
        //--------------------------------------------------------------
        Tsukino::Core::Path assetRoot     = Tsukino::IO::FileSystem::GetAssetRootPath();
        Tsukino::Core::Path fullInputPath = assetRoot / inputPath;

        CubemapDesc desc;
        {
            std::ifstream ifs(fullInputPath.string());
            if(!ifs) {
                Tsukino::Core::Log::Error("CubemapImporter: Failed to open: " + fullInputPath.string());
                return false;
            }
            cereal::JSONInputArchive archive(ifs);
            archive(desc);
        }

        // 面の順番：DirectXキューブマップ規約
        // +X, -X, +Y, -Y, +Z, -Z
        const std::array<std::string, 6> facePaths = {desc.px, desc.nx, desc.py, desc.ny, desc.pz, desc.nz};
        const std::array<std::string, 6> faceKeys  = {"px", "nx", "py", "ny", "pz", "nz"};

        //--------------------------------------------------------------
        // 6枚の画像を読み込む
        //--------------------------------------------------------------
        std::array<DirectX::ScratchImage, 6> faceImages;
        size_t                               width  = 0;
        size_t                               height = 0;

        for(u32 i = 0; i < 6; ++i) {
            if(facePaths[i].empty()) {
                Tsukino::Core::Log::Error("CubemapImporter: Missing face path: " + faceKeys[i]);
                return false;
            }

            Tsukino::Core::Path faceFullPath = assetRoot / Tsukino::Core::Path(facePaths[i]);

            DirectX::TexMetadata metadata;
            HRESULT              hr = DirectX::LoadFromWICFile(faceFullPath.ToWString().c_str(), DirectX::WIC_FLAGS_NONE, &metadata, faceImages[i]);

            if(FAILED(hr)) {
                Tsukino::Core::Log::Error("CubemapImporter: Failed to load face: " + faceFullPath.string());
                return false;
            }

            // 最初の面でサイズを確定
            if(i == 0) {
                width  = metadata.width;
                height = metadata.height;
            } else {
                if(metadata.width != width || metadata.height != height) {
                    Tsukino::Core::Log::Error("CubemapImporter: Face size mismatch at: " + faceKeys[i]);
                    return false;
                }
            }

            // RGBA8に統一
            const DirectX::Image* srcImg = faceImages[i].GetImage(0, 0, 0);
            if(srcImg->format != DXGI_FORMAT_R8G8B8A8_UNORM) {
                DirectX::ScratchImage converted;
                HRESULT hrConv = DirectX::Convert(*srcImg, DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, converted);
                if(FAILED(hrConv)) {
                    Tsukino::Core::Log::Error("CubemapImporter: Failed to convert face format: " + faceKeys[i]);
                    return false;
                }
                faceImages[i] = std::move(converted);
            }
        }

        //--------------------------------------------------------------
        // 4の倍数にリサイズ
        //--------------------------------------------------------------
        size_t newWidth  = (width + 3) & ~3;
        size_t newHeight = (height + 3) & ~3;

        if(newWidth != width || newHeight != height) {
            for(u32 i = 0; i < 6; ++i) {
                DirectX::ScratchImage resized;
                DirectX::Resize(*faceImages[i].GetImage(0, 0, 0), newWidth, newHeight, DirectX::TEX_FILTER_DEFAULT, resized);
                faceImages[i] = std::move(resized);
            }
            width  = newWidth;
            height = newHeight;
        }

        //--------------------------------------------------------------
        // 6枚をキューブマップにまとめる
        //--------------------------------------------------------------
        DirectX::ScratchImage cubeImage;
        HRESULT               hr = cubeImage.InitializeCube(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1);
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("CubemapImporter: Failed to initialize cubemap.");
            return false;
        }

        for(u32 i = 0; i < 6; ++i) {
            const DirectX::Image* src = faceImages[i].GetImage(0, 0, 0);
            const DirectX::Image* dst = cubeImage.GetImage(0, i, 0);
            std::memcpy(dst->pixels, src->pixels, src->slicePitch);
        }

        //--------------------------------------------------------------
        // ミップマップ生成
        //--------------------------------------------------------------
        DirectX::ScratchImage mipChain;
        hr = DirectX::GenerateMipMaps(cubeImage.GetImages(), cubeImage.GetImageCount(), cubeImage.GetMetadata(), DirectX::TEX_FILTER_DEFAULT, 0, mipChain);

        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("CubemapImporter: Failed to generate mipmaps.");
            return false;
        }

        //--------------------------------------------------------------
        // BC1圧縮（スカイボックスはアルファ不要）
        //--------------------------------------------------------------
        DirectX::ScratchImage compressed;
        hr = DirectX::Compress(mipChain.GetImages(),
                               mipChain.GetImageCount(),
                               mipChain.GetMetadata(),
                               DXGI_FORMAT_BC1_UNORM,
                               DirectX::TEX_COMPRESS_DEFAULT,
                               DirectX::TEX_THRESHOLD_DEFAULT,
                               compressed);

        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("CubemapImporter: Failed to compress cubemap.");
            return false;
        }

        //--------------------------------------------------------------
        // 出力パスの構築・保存
        //--------------------------------------------------------------
        Tsukino::Core::Path tempPath = inputPath;
        tempPath.replace_extension(".tcc");
        Tsukino::Core::Path outputPath = outputDirectory / tempPath;

        Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path());

        hr = DirectX::SaveToDDSFile(
            compressed.GetImages(), compressed.GetImageCount(), compressed.GetMetadata(), DirectX::DDS_FLAGS_NONE, outputPath.ToWString().c_str());

        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("CubemapImporter: Failed to save DDS: " + outputPath.string());
            return false;
        }

        Tsukino::Core::Log::Info("CubemapImporter: Successfully imported cubemap: " + outputPath.string());
        return true;
    }

}    // namespace Tsukino::Asset

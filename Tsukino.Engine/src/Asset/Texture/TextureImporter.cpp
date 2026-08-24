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

#include <cstdint>
#include <cstring>
#include <vector>

namespace {
    //--------------------------------------------------------------
    //! @brief  完全に透明なピクセルのRGBを、近傍の不透明なピクセルの色で塗り広げる。
    //!         バイリニア補間サンプリング時に透明ピクセルの黒いRGBが不透明ピクセルの
    //!         色と混ざり込み、スプライトの端が黒く滲んで見える問題への対策。
    //--------------------------------------------------------------
    void DilateTransparentEdges(DirectX::ScratchImage& image) {
        const DirectX::Image* img = image.GetImage(0, 0, 0);
        if(!img) {
            return;
        }

        // 1ピクセル4バイト・8bit/チャンネルの形式以外は対象外（安全側に倒す）。
        // LoadFromWICFileはWIC_FLAGS_FORCE_RGBを指定しない限りB8G8R8A8のまま返すため、
        // RGBA8/BGRA8の両方を許可する（アルファは常に末尾バイトのため、
        // R/Bのチャンネル順はにじみ拡張アルゴリズム自体には影響しない）。
        switch(img->format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            break;
        default:
            return;
        }

        const size_t width    = img->width;
        const size_t height   = img->height;
        const size_t rowPitch = img->rowPitch;

        auto pixelAt = [rowPitch](uint8_t* base, size_t x, size_t y) -> uint8_t* {
            return base + y * rowPitch + x * 4;
        };

        std::vector<uint8_t> work(img->pixels, img->pixels + img->slicePitch);

        // バイリニア補間の影響範囲をカバーするため複数回にわたり外側へ色を伝播させる
        constexpr int kIterations = 3;
        for(int iter = 0; iter < kIterations; ++iter) {
            const std::vector<uint8_t> src = work;

            for(size_t y = 0; y < height; ++y) {
                for(size_t x = 0; x < width; ++x) {
                    const uint8_t* srcPixel = pixelAt(const_cast<uint8_t*>(src.data()), x, y);
                    if(srcPixel[3] != 0) {
                        continue;    // 不透明（もしくは半透明）ピクセルはそのまま維持
                    }

                    uint32_t sumR = 0, sumG = 0, sumB = 0, count = 0;
                    for(int dy = -1; dy <= 1; ++dy) {
                        for(int dx = -1; dx <= 1; ++dx) {
                            if(dx == 0 && dy == 0) {
                                continue;
                            }

                            const int64_t nx = static_cast<int64_t>(x) + dx;
                            const int64_t ny = static_cast<int64_t>(y) + dy;
                            if(nx < 0 || ny < 0 || nx >= static_cast<int64_t>(width) || ny >= static_cast<int64_t>(height)) {
                                continue;
                            }

                            const uint8_t* neighbor = pixelAt(const_cast<uint8_t*>(src.data()), static_cast<size_t>(nx), static_cast<size_t>(ny));
                            if(neighbor[3] == 0) {
                                continue;    // 近傍も透明なら色の供給元にしない
                            }

                            sumR += neighbor[0];
                            sumG += neighbor[1];
                            sumB += neighbor[2];
                            ++count;
                        }
                    }

                    if(count == 0) {
                        continue;    // 今回は塗れなかった（次のイテレーションで伝播を試みる）
                    }

                    uint8_t* dstPixel = pixelAt(work.data(), x, y);
                    dstPixel[0]       = static_cast<uint8_t>(sumR / count);
                    dstPixel[1]       = static_cast<uint8_t>(sumG / count);
                    dstPixel[2]       = static_cast<uint8_t>(sumB / count);
                    // alpha(dstPixel[3])は0のまま変更しない
                }
            }
        }

        memcpy(img->pixels, work.data(), img->slicePitch);
    }
}    // namespace

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
        DilateTransparentEdges(image);

        //--------------------------------------------------------------
        // outputDirectory(Cache/) に inputPath(相対) を結合して階層を維持
        // (inputPathが絶対パスの場合はToEngineRelativePath()で相対パスに戻してから結合する)
        //--------------------------------------------------------------
        Tsukino::Core::Path outputPath = outputDirectory / Tsukino::IO::FileSystem::ToEngineRelativePath(inputPath);
        outputPath.replace_extension(".dds");    // 拡張子をDDSに変更

        //--------------------------------------------------------------
        // 親ディレクトリの生成
        //--------------------------------------------------------------
        Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path());

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

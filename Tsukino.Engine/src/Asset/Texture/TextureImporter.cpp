//--------------------------------------------------------------
//! @file   TextureImporter.cpp
//! @brief  テクスチャのインポータークラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Texture/TextureImporter.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include "ImageCodec.hpp"

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>

#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {

    namespace {
        //--------------------------------------------------------------
        //! 線形のまま扱うべき（色ではない）テクスチャかを、ファイル名の末尾で判定します。
        //! 法線・メタリック/ラフネス・AO などは値そのものに意味があり、
        //! sRGB として読むとガンマで値が歪むため、名前の慣習（_n / _normal など）で見分けます。
        //! @param  [in] path テクスチャのパス
        //! @return 線形として扱うなら true
        //--------------------------------------------------------------
        bool IsLinearTexture(const Tsukino::Core::Path& path) {
            std::string stem = path.stem();
            for(char& c : stem) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            static const char* const kLinearSuffixes[] = {
                "_n",
                "_nrm",
                "_norm",
                "_normal",
                "_mr",
                "_orm",
                "_ao",
                "_metallic",
                "_roughness",
                "_rough",
                "_height",
                "_disp",
                "_mask",
            };
            for(const char* suffix : kLinearSuffixes) {
                const size_t length = std::strlen(suffix);
                if(stem.size() >= length && stem.compare(stem.size() - length, length, suffix) == 0) {
                    return true;
                }
            }
            return false;
        }

        //--------------------------------------------------------------
        //! sRGB の印を外した形式を返します（同じ圧縮かを比べるため）。
        //! @param  [in] format 形式
        //! @return sRGB の印を外した形式
        //--------------------------------------------------------------
        Tsukino::GraphicsCommon::TextureFormat WithoutSrgb(Tsukino::GraphicsCommon::TextureFormat format) {
            using Tsukino::GraphicsCommon::TextureFormat;
            switch(format) {
            case TextureFormat::RGBA8_sRGB:
                return TextureFormat::RGBA8;
            case TextureFormat::BGRA8_sRGB:
                return TextureFormat::BGRA8;
            case TextureFormat::BC1_sRGB:
                return TextureFormat::BC1;
            case TextureFormat::BC3_sRGB:
                return TextureFormat::BC3;
            case TextureFormat::BC7_sRGB:
                return TextureFormat::BC7;
            case TextureFormat::ASTC4x4_sRGB:
                return TextureFormat::ASTC4x4;
            case TextureFormat::ASTC6x6_sRGB:
                return TextureFormat::ASTC6x6;
            default:
                return format;
            }
        }
    }    // namespace

    //--------------------------------------------------------------
    //! 既にあるキャッシュが規則のフォーマットと違うかを調べます。
    //--------------------------------------------------------------
    bool TextureImporter::IsCacheOutdated(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& cachePath) const {
        const TextureImportRule* rule = m_rules ? m_rules->Find(inputPath) : nullptr;
        if(!rule)
            return false;

        // ヘッダだけ読むので、枚数が多くても起動はほとんど遅くならない
        constexpr std::size_t                  kHeaderBytes = 4096;
        const std::vector<std::uint8_t>        head         = Tsukino::IO::FileSystem::ReadBinaryHead(cachePath, kHeaderBytes);
        Tsukino::GraphicsCommon::TextureFormat format       = Tsukino::GraphicsCommon::TextureFormat::Unknown;
        u32                                    mipCount     = 0;
        if(!Detail::ReadTextureFileInfo(head.data(), head.size(), format, mipCount))
            return true;

        if(rule->generateMips && mipCount <= 1)
            return true;

        // BC7 は BC3 で作るので、BC7 の規則は BC3 のキャッシュと一致とみなす。スマホ向けなら ASTC と比べる
        Tsukino::GraphicsCommon::TextureFormat wanted = m_rules->Resolve(WithoutSrgb(rule->compressedFormat));
        if(wanted == Tsukino::GraphicsCommon::TextureFormat::BC7)
            wanted = Tsukino::GraphicsCommon::TextureFormat::BC3;
        return wanted != Tsukino::GraphicsCommon::TextureFormat::Unknown && WithoutSrgb(format) != wanted;
    }

    //--------------------------------------------------------------
    //! @brief  テクスチャのインポート関数
    //--------------------------------------------------------------
    bool TextureImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        //--------------------------------------------------------------
        // ルートからの絶対パスで開いて、RGBA8 にする
        //--------------------------------------------------------------
        const Tsukino::Core::Path       absoluteInputPath = Tsukino::IO::FileSystem::GetAssetRootPath() / inputPath;
        const std::vector<std::uint8_t> fileData          = Tsukino::IO::FileSystem::ReadBinary(absoluteInputPath);

        Detail::Rgba8Image image;
        if(!Detail::DecodeImage(fileData.data(), fileData.size(), image)) {
            Tsukino::Core::Log::Error("Failed to load texture: " + absoluteInputPath.string());
            return false;
        }

        //--------------------------------------------------------------
        // 透明ピクセルのRGBを近傍色で塗り広げる（バイリニア補間時の黒縁対策）
        //--------------------------------------------------------------
        Detail::DilateTransparentEdges(image);

        //--------------------------------------------------------------
        // outputDirectory(Cache/) に inputPath(相対) を結合して階層を維持
        // (inputPathが絶対パスの場合はToEngineRelativePath()で相対パスに戻してから結合する)
        //--------------------------------------------------------------
        Tsukino::Core::Path outputPath = outputDirectory / Tsukino::IO::FileSystem::ToEngineRelativePath(inputPath);
        outputPath.replace_extension(".ktx");

        //--------------------------------------------------------------
        // 色のテクスチャは sRGB、法線などのデータのテクスチャは線形として扱う（ここが最重要）。
        // KTX は色空間を持てるので、読むときに sRGB の値を線形の値と取り違えない。
        // 規則があるテクスチャは、ミップマップを作ってブロック圧縮してから保存する
        //--------------------------------------------------------------
        const TextureImportRule* rule = m_rules ? m_rules->Find(inputPath) : nullptr;

        Detail::ImageEncodeOptions options;
        options.srgb             = !IsLinearTexture(inputPath);
        options.generateMips     = rule && rule->generateMips;
        options.compressedFormat = rule ? m_rules->Resolve(WithoutSrgb(rule->compressedFormat)) : Tsukino::GraphicsCommon::TextureFormat::Unknown;

        if(!Detail::WriteTextureKtx(outputPath, image, options)) {
            Tsukino::Core::Log::Error("Failed to save texture: " + outputPath.string());
            return false;
        }

        Tsukino::Core::Log::Info("Texture imported: " + outputPath.string());
        return true;
    }

}    // namespace Tsukino::Asset

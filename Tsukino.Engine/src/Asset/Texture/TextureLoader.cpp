//--------------------------------------------------------------
//! @file   TextureLoader.cpp
//! @brief  テクスチャアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Texture/TextureLoader.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include "ImageCodec.hpp"

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <cstdint>
#include <string>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    namespace {
        //! ブロック圧縮の形式かを返します（最上段の大きさに4の倍数の決まりがある）。
        bool IsBlockCompressed(Tsukino::GraphicsCommon::TextureFormat format) {
            using Tsukino::GraphicsCommon::TextureFormat;
            switch(format) {
            case TextureFormat::BC1:
            case TextureFormat::BC1_sRGB:
            case TextureFormat::BC3:
            case TextureFormat::BC3_sRGB:
            case TextureFormat::BC4:
            case TextureFormat::BC5:
            case TextureFormat::BC7:
            case TextureFormat::BC7_sRGB: return true;
            default:                      return false;
            }
        }
    }    // namespace

    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する
    //--------------------------------------------------------------
    bool TextureLoader::CanLoad(const std::string& ext) const {
        return ext == ".ktx";
    }

    //--------------------------------------------------------------
    //! @brief ファイルを読み込みTextureAssetを生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> TextureLoader::Load(const Tsukino::Core::Path& path) {
        const std::vector<std::uint8_t> fileData = Tsukino::IO::FileSystem::ReadBinary(path);

        Detail::TextureFileData file;
        if(!Detail::ReadTextureFile(fileData.data(), fileData.size(), file)) {
            return nullptr;
        }

        const size_t mipLevels = file.mips.size();

        //--------------------------------------------------------------
        // 許されたテクスチャは上位の段を捨てて小さく持つ（描画品質を下げて動かすときのメモリ節約）。
        // ブロック圧縮の最上段は4の倍数の大きさでないとテクスチャを作れないので、そこで止める
        //--------------------------------------------------------------
        size_t firstMip = 0;
        if(const TextureImportRule* rule = m_rules ? m_rules->Find(path) : nullptr; rule && rule->allowMipSkip) {
            const size_t wanted            = (std::min)(static_cast<size_t>(m_rules->GetMipSkip()), mipLevels - 1);
            const bool   isBlockCompressed = IsBlockCompressed(file.format);
            while(firstMip < wanted) {
                const Detail::TextureFileData::Mip& next = file.mips[firstMip + 1];
                if(isBlockCompressed && (next.width % 4 != 0 || next.height % 4 != 0))
                    break;
                ++firstMip;
            }
        }

        // TextureAsset を作成
        auto asset    = Tsukino::Core::CreateRef<TextureAsset>();
        asset->width  = file.mips[firstMip].width;
        asset->height = file.mips[firstMip].height;
        asset->format = file.format;

        // ファイルが持つ全段を最上段から順に詰めて写す。
        // 最上段だけ写すと、圧縮済みの絵（モデル埋め込みのBC3など）はGPUで縮められないので、
        // 遠くでも元の大きさのまま読まれて帯域を食い、細かい模様がちらつく
        const size_t skipBytes = file.mips[firstMip].offset;
        for(size_t mip = firstMip; mip < mipLevels; ++mip) {
            asset->mips.push_back({file.mips[mip].offset - skipBytes, file.mips[mip].rowPitch});
        }
        asset->pixels.assign(file.pixels.begin() + static_cast<std::ptrdiff_t>(skipBytes), file.pixels.end());

        // 1段だけの絵は、縮めて描くと細い線が欠けてちらつくので、GPUへ送るときにミップマップを作らせる。
        // 作れるかどうか（非圧縮のRGBA8系か）はRenderer側が見る
        asset->generateMips = asset->mips.size() <= 1;

        return asset;
    }

}    // namespace Tsukino::Asset

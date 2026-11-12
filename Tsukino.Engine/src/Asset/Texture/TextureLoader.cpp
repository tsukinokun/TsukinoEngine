//--------------------------------------------------------------
//! @file   TextureLoader.cpp
//! @brief  テクスチャアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Texture/TextureLoader.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include "TextureFormatDxgi.hpp"

#include <Tsukino/Core/Log.hpp>

#include <DirectXTex/DirectXTex.h>

#include <algorithm>
#include <cstring>
#include <string>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する
    //--------------------------------------------------------------
    bool TextureLoader::CanLoad(const std::string& ext) const {
        return ext == ".dds";
    }

    //--------------------------------------------------------------
    //! @brief ファイルを読み込みTextureAssetを生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> TextureLoader::Load(const Tsukino::Core::Path& path) {
        DirectX::ScratchImage image;

        // DDS 読み込み
        auto hr = DirectX::LoadFromDDSFile(path.ToWString().c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);

        // 読み込みに失敗した場合は nullptr を返す
        if(FAILED(hr)) {
            return nullptr;
        }

        //--------------------------------------------------------------
        // エンジンが扱わないフォーマット（B5G6R5 などの珍しい並び）は、RGBA8 に直してから持つ。
        // 色空間（sRGB / 線形）はそのまま引き継ぐ
        //--------------------------------------------------------------
        if(Detail::FromDxgiFormat(image.GetMetadata().format) == Tsukino::GraphicsCommon::TextureFormat::Unknown) {
            const DXGI_FORMAT     srcFormat = image.GetMetadata().format;
            const DXGI_FORMAT     dstFormat = DirectX::IsSRGB(srcFormat) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
            DirectX::ScratchImage converted;
            const HRESULT         hrConv    = DirectX::IsCompressed(srcFormat)
                                                  ? DirectX::Decompress(image.GetImages(), image.GetImageCount(), image.GetMetadata(), dstFormat, converted)
                                                  : DirectX::Convert(image.GetImages(), image.GetImageCount(), image.GetMetadata(), dstFormat,
                                                                     DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, converted);
            if(FAILED(hrConv)) {
                Tsukino::Core::Log::Error("TextureLoader: Unsupported pixel format " + std::to_string(static_cast<int>(srcFormat)) + ": " + path.string());
                return nullptr;
            }
            Tsukino::Core::Log::Warn("TextureLoader: Converted pixel format " + std::to_string(static_cast<int>(srcFormat)) + " to RGBA8: " + path.string());
            image = std::move(converted);
        }

        const size_t mipLevels = (std::max)(size_t{ 1 }, image.GetMetadata().mipLevels);

        //--------------------------------------------------------------
        // 許されたテクスチャは上位の段を捨てて小さく持つ（描画品質を下げて動かすときのメモリ節約）。
        // ブロック圧縮の最上段は4の倍数の大きさでないとテクスチャを作れないので、そこで止める
        //--------------------------------------------------------------
        size_t firstMip = 0;
        if(const TextureImportRule* rule = m_rules ? m_rules->Find(path) : nullptr; rule && rule->allowMipSkip) {
            const size_t wanted = (std::min)(static_cast<size_t>(m_rules->GetMipSkip()), mipLevels - 1);
            const bool   isBlockCompressed = DirectX::IsCompressed(image.GetMetadata().format);
            while(firstMip < wanted) {
                const DirectX::Image* next = image.GetImage(firstMip + 1, 0, 0);
                if(!next || (isBlockCompressed && (next->width % 4 != 0 || next->height % 4 != 0)))
                    break;
                ++firstMip;
            }
        }

        const DirectX::Image* img = image.GetImage(firstMip, 0, 0);
        if(!img)
            return nullptr;

        // TextureAsset を作成
        auto asset    = Tsukino::Core::CreateRef<TextureAsset>();
        asset->width  = static_cast<uint32_t>(img->width);
        asset->height = static_cast<uint32_t>(img->height);
        asset->format = Detail::FromDxgiFormat(img->format);

        // 配列・キューブの先頭1枚について、ファイルが持つ全段を最上段から順に詰めて写す。
        // 最上段だけ写すと、圧縮済みの絵（モデル埋め込みのBC3など）はGPUで縮められないので、
        // 遠くでも元の大きさのまま読まれて帯域を食い、細かい模様がちらつく
        size_t total = 0;
        for(size_t mip = firstMip; mip < mipLevels; ++mip) {
            const DirectX::Image* level = image.GetImage(mip, 0, 0);
            if(!level)
                break;
            asset->mips.push_back({ total, static_cast<uint32_t>(level->rowPitch) });
            total += level->slicePitch;
        }

        asset->pixels.resize(total);
        for(size_t mip = 0; mip < asset->mips.size(); ++mip) {
            const DirectX::Image* level = image.GetImage(firstMip + mip, 0, 0);
            memcpy(asset->pixels.data() + asset->mips[mip].offset, level->pixels, level->slicePitch);
        }

        // 1段だけの絵は、縮めて描くと細い線が欠けてちらつくので、GPUへ送るときにミップマップを作らせる。
        // 作れるかどうか（非圧縮のRGBA8系か）はRenderer側が見る
        asset->generateMips = asset->mips.size() <= 1;

        // 成功した場合はassetを返す
        return asset;
    }

}    // namespace Tsukino::Asset

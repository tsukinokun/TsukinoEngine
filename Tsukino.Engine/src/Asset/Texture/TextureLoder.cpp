//--------------------------------------------------------------
//! @file   TextureLoder.cpp
//! @brief  テクスチャアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Texture/TextureLoder.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include <DirectXTex/DirectXTex.h>

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

        const DirectX::Image* img = image.GetImage(0, 0, 0);
        if(!img)
            return nullptr;

        // TextureAsset を作成
        auto asset    = Tsukino::Core::CreateRef<TextureAsset>();
        asset->width  = img->width;
        asset->height = img->height;
        asset->format = img->format;

        // ピクセルコピー
        size_t size = img->slicePitch;
        asset->pixels.resize(size);
        memcpy(asset->pixels.data(), img->pixels, size);

        // 成功した場合はassetを返す
        return asset;
    }

}    // namespace Tsukino::Asset

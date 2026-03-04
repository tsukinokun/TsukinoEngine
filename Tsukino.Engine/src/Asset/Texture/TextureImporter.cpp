//--------------------------------------------------------------
//! @file   TextureImporter.cpp
//! @brief  テクスチャのインポータークラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Texture/TextureImporter.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include <Tsukino/Core/Log.hpp>

#include <DirectXTex/DirectXTex.h>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief  テクスチャのインポート関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> TextureImporter::Import(const Tsukino::Core::Path& path) {
        DirectX::ScratchImage image;       // ピクセルデータを格納するオブジェクト
        DirectX::TexMetadata  metadata;    // 画像のメタデータを格納するオブジェクト

        // 画像ファイルを読み込む（PNG/JPG/TGA など WIC 対応フォーマット）
        HRESULT hr = DirectX::LoadFromWICFile(path.ToWString().c_str(), DirectX::WIC_FLAGS_NONE, &metadata, image);

        if(FAILED(hr)) {
            // 読み込み失敗
            Tsukino::Core::Log::Error("Failed to load texture: " + path.string());
            return nullptr;
        }

        // TextureAsset を生成
        auto asset    = Tsukino::Core::CreateRef<TextureAsset>();    // TextureAssetを生成
        asset->width  = static_cast<uint32_t>(metadata.width);       // 画像の幅をTextureAssetにセット
        asset->height = static_cast<uint32_t>(metadata.height);      // 画像の高さをTextureAssetにセット
        asset->format = metadata.format;                             // 画像のフォーマットをTextureAssetにセット

        // ピクセルデータをコピー
        const uint8_t* src  = image.GetPixels();        // ピクセルデータの先頭アドレス
        size_t         size = image.GetPixelsSize();    // ピクセルデータのサイズ（バイト単位）
        asset->pixels.assign(src, src + size);          // ピクセルデータをTextureAssetのpixelsベクターにコピー

        // アセットを返す
        return asset;
    }

}    // namespace Tsukino::Asset

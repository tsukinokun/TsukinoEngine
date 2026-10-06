//----------------------------------------------------------------------------
//! @file   MaterialAssetBuilder.cpp
//! @brief  マテリアルデータからマテリアルアセットを組み立てる関数の実装
//----------------------------------------------------------------------------
#include <Tsukino/Engine/Asset/Material/MaterialAssetBuilder.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Material/MaterialAsset.hpp>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {

    //----------------------------------------------------------------------------
    //! マテリアルデータからマテリアルアセットを組み立てます。
    //----------------------------------------------------------------------------
    Tsukino::Core::Ref<MaterialAsset> BuildMaterialAsset(AssetManager& assetManager, const Tsukino::GraphicsCommon::MaterialData& data) {
        auto asset  = Tsukino::Core::CreateRef<MaterialAsset>();
        asset->data = data;

        // パスのあるテクスチャだけを読み込む（空のスロットは描画時に既定のテクスチャへ置き換わる）
        auto load = [&](const std::string& path) { return path.empty() ? AssetHandle() : assetManager.Load(Tsukino::Core::Path(path)); };
        asset->albedoHandle            = load(data.albedoMap);
        asset->normalHandle            = load(data.normalMap);
        asset->metallicRoughnessHandle = load(data.metallicRoughnessMap);
        asset->emissiveHandle          = load(data.emissiveMap);
        asset->aoHandle                = load(data.aoMap);
        return asset;
    }
}    // namespace Tsukino::Asset

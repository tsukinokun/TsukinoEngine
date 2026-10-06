//----------------------------------------------------------------------------
//! @file   MaterialAssetBuilder.hpp
//! @brief  マテリアルデータからマテリアルアセットを組み立てる関数
//! @detail モデルに含まれるマテリアル（ModelLoader）と、単体のマテリアルファイル .tmat（MaterialLoader）の
//!         どちらも、同じ手順（テクスチャを読み込んでハンドルを持たせる）で MaterialAsset にします。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Memory.hpp>
#include <Tsukino/GraphicsCommon/Material/MaterialData.hpp>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    class AssetManager;     // 前方宣言
    class MaterialAsset;    // 前方宣言

    //! マテリアルデータからマテリアルアセットを組み立てます。テクスチャのパスがあれば読み込み、ハンドルを持たせます。
    //! ハンドルの設定と AssetManager への登録は呼び出し側が行います。
    //! @param  [in] assetManager テクスチャの読み込みに使う AssetManager
    //! @param  [in] data         マテリアルデータ
    //! @return 組み立てたマテリアルアセット
    Tsukino::Core::Ref<MaterialAsset> BuildMaterialAsset(AssetManager& assetManager, const Tsukino::GraphicsCommon::MaterialData& data);
}    // namespace Tsukino::Asset

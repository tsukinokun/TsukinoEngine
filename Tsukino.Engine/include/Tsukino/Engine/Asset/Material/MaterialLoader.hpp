//----------------------------------------------------------------------------
//! @file   MaterialLoader.hpp
//! @brief  マテリアルファイル（.tmat）のキャッシュを読み込むローダー
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetLoader.hpp>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    class AssetManager;    // 前方宣言

    //! マテリアルファイル（.tmat）のキャッシュを読み込み、MaterialAsset を生成するローダーです。
    //! テクスチャも読み込むため、AssetManager を受け取ります（ModelLoader と同じ）。
    class MaterialLoader : public IAssetLoader {
    public:

        //! コンストラクタです。
        //! @param  [in] assetManager テクスチャの読み込みに使う AssetManager
        explicit MaterialLoader(AssetManager* assetManager)
            : m_assetManager(assetManager) {}

        //! 対応する拡張子か判定します。
        //! @param  [in] ext 拡張子
        //! @return 対応している場合は true
        [[nodiscard]]
        bool CanLoad(const std::string& ext) const override;

        //! .tmat のキャッシュを読み込み、MaterialAsset を生成します。
        //! @param  [in] path .tmat のキャッシュファイルのパス
        //! @return 読み込まれた MaterialAsset（失敗時は nullptr）
        [[nodiscard]]
        Tsukino::Core::Ref<IAsset> Load(const Tsukino::Core::Path& path) override;

    private:
        AssetManager* m_assetManager = nullptr;    // テクスチャの読み込みに使う（非所有）
    };
}    // namespace Tsukino::Asset

//--------------------------------------------------------------
//! @file   ModelLoader.hpp
//! @brief  モデルアセットローダーの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetLoader.hpp>
#include <Tsukino/Core/Path.hpp>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    // 前方宣言
    class AssetManager;

    //--------------------------------------------------------------
    //! @class  ModelLoader
    //! @brief  .shader ファイルを読み込んでShaderAssetを生成するローダー
    //--------------------------------------------------------------
    class ModelLoader : public IAssetLoader {
    public:
        //--------------------------------------------------------------
        //! @brief コンストラクタ
        //! @param assetManager [in] テクスチャロードに使用するAssetManager
        //--------------------------------------------------------------
        explicit ModelLoader(AssetManager* assetManager)
            : m_assetManager(assetManager) {}

        //--------------------------------------------------------------
        // 対応する拡張子か判定する
        //! @param ext [in] 拡張子
        //! @return 対応している場合 true
        //--------------------------------------------------------------
        [[nodiscard]]
        bool CanLoad(const std::string& ext) const override;

        //--------------------------------------------------------------
        // モデルファイルを読み込みShaderAssetを生成する
        //! @param path [in] モデルファイルのパス
        //! @return 読み込まれたShaderAsset
        //--------------------------------------------------------------
        [[nodiscard]]
        Tsukino::Core::Ref<IAsset> Load(const Tsukino::Core::Path& path) override;

    private:
        AssetManager* m_assetManager = nullptr;
    };

}    // namespace Tsukino::Asset

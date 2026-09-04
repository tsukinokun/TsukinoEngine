//--------------------------------------------------------------
//! @file   TextureLoader.hpp
//! @brief  テクスチャアセットローダーの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetLoader.hpp>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  TextureLoader
    //! @brief  テクスチャアセットのローダー
    //--------------------------------------------------------------
    class TextureLoader : public IAssetLoader {
    public:
        //--------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //--------------------------------------------------------------
        TextureLoader() = default;

        //--------------------------------------------------------------
        // 対応する拡張子か判定する
        //! @param  ext [in] 拡張子
        //! @return 対応している場合はtrue
        //--------------------------------------------------------------
        [[nodiscard]]
        bool CanLoad(const std::string& ext) const override;

        //--------------------------------------------------------------
        // テクスチャファイルを読み込みTextureAssetを生成する
        //! @param  path [in] テクスチャファイルのパス
        //! @return 読み込まれたTextureAsset
        //--------------------------------------------------------------
        [[nodiscard]]
        Tsukino::Core::Ref<IAsset> Load(const Tsukino::Core::Path& path) override;
    };

}    // namespace Tsukino::Asset

//--------------------------------------------------------------
//! @file	TextureImporter.hpp
//! @brief  テクスチャのインポータークラス
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetImporter.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class TextureImporter
    //! @brief テクスチャのインポータークラス
    //--------------------------------------------------------------
    class TextureImporter : public IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! @brief  テクスチャアセットをインポートする関数
        //! @param  path [in] インポートするテクスチャアセットのパス
        //! @return インポートされたテクスチャアセットの参照
        //--------------------------------------------------------------
        Tsukino::Core::Ref<IAsset> Import(const Tsukino::Core::Path& path) override;
    };

}    // namespace Tsukino::Asset

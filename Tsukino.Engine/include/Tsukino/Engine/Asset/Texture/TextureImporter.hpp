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
        //! @param  path            [in] インポートするテクスチャアセットのパス
        //! @param  outPutDirectory [in] 出力先ディレクトリ
        //! @return インポートされたテクスチャアセットの参照
        //--------------------------------------------------------------
        bool Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) override;
    };

}    // namespace Tsukino::Asset

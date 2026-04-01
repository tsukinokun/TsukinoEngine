//--------------------------------------------------------------
//! @file	FontImporter.hpp
//! @brief  フォントのインポータークラス
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetImporter.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class FontImporter
    //! @brief フォントのインポータークラス
    //--------------------------------------------------------------
    class FontImporter : public IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! @brief  フォントアセットをインポートする関数
        //! @param  path            [in] インポートするフォントアセットのパス
        //! @param  outPutDirectory [in] 出力先ディレクトリ
        //! @return インポートされたフォントアセットの参照
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) override;
    };

}    // namespace Tsukino::Asset

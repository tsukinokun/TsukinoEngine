//--------------------------------------------------------------
//! @file	DynamicFontImporter.hpp
//! @brief  動的フォント(.dfont)のインポータークラス
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetImporter.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class DynamicFontImporter
    //! @brief .dfont設定を読み取り、ttf/otfの生データをキャッシュへコピーするインポーター
    //! @note  MakeSpriteFont.exeのような事前ベイクは行わない
    //!        (グリフは実行時にオンデマンドでラスタライズされる)
    //--------------------------------------------------------------
    class DynamicFontImporter : public IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! @brief  動的フォントアセットをインポートする関数
        //! @param  inPutPath       [in] インポートする.dfontファイルのパス
        //! @param  outPutDirectory [in] 出力先ディレクトリ
        //! @return 成功した場合は true
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) override;
    };

}    // namespace Tsukino::Asset

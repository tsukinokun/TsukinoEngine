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
    //! @note  文字の画像を事前に焼き込むことはしない
    //!        (グリフは実行時にオンデマンドでラスタライズされる)
    //--------------------------------------------------------------
    class DynamicFontImporter : public IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! @brief  動的フォントアセットをインポートする関数
        //! @param  inputPath       [in] インポートする.dfontファイルのパス
        //! @param  outputDirectory [in] 出力先ディレクトリ
        //! @return 成功した場合は true
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) override;
    };

}    // namespace Tsukino::Asset

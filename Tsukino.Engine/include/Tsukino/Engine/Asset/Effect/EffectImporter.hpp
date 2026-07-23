//--------------------------------------------------------------
//! @file   EffectImporter.hpp
//! @brief  エフェクトインポーターの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetImporter.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  EffectImporter
    //! @brief  .efk ファイルを Cache にコピーする最小インポーター
    //--------------------------------------------------------------
    class EffectImporter : public IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //--------------------------------------------------------------
        EffectImporter() = default;

        //--------------------------------------------------------------
        //! @brief ソースの .efk を出力先ディレクトリにコピーする
        //! @param  inPutPath        [in] インポート元のパス
        //! @param  outPutDirectory  [in] 出力先ディレクトリ（Cache/）
        //! @return 成功時 true
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) override;
    };

}    // namespace Tsukino::Asset

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
        //! @param  inputPath        [in] インポート元のパス
        //! @param  outputDirectory  [in] 出力先ディレクトリ（Cache/）
        //! @return 成功時 true
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) override;
    };

}    // namespace Tsukino::Asset

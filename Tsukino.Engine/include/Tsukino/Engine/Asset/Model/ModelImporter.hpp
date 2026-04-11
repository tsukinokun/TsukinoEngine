//--------------------------------------------------------------
//! @file	ModelImporter.hpp
//! @brief  モデルのインポータークラス
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetImporter.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class ModelImporter
    //! @brief モデルのインポータークラス
    //--------------------------------------------------------------
    class ModelImporter : public IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! @brief  モデルアセットをインポートする関数
        //! @param  path            [in] インポートするモデルアセットのパス
        //! @param  outPutDirectory [in] 出力先ディレクトリ
        //! @return インポートされたモデルアセットの参照
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) override;
    };

}    // namespace Tsukino::Asset

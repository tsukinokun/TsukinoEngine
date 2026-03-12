//--------------------------------------------------------------
//! @file	ShaderImporter.hpp
//! @brief  シェーダーのインポータークラス
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetImporter.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class ShaderImporter
    //! @brief シェーダーのインポータークラス
    //--------------------------------------------------------------
    class ShaderImporter : public IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! @brief  シェーダーアセットをインポートする関数
        //! @param  path            [in] インポートするシェーダーアセットのパス
        //! @param  outPutDirectory [in] 出力先ディレクトリ
        //! @return インポートされたシェーダーアセットの参照
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) override;
    };

}    // namespace Tsukino::Asset

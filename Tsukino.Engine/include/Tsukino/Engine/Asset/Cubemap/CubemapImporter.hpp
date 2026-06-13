//--------------------------------------------------------------
//! @file   CubemapImporter.hpp
//! @brief  キューブマップのインポータークラス
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetImporter.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class CubemapImporter
    //! @brief .cubemapファイルから6枚のPNGを読み込み
    //!        キューブマップDDSとして出力するインポータークラス
    //--------------------------------------------------------------
    class CubemapImporter : public IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! @brief  キューブマップアセットをインポートする関数
        //! @param  inputPath       [in] .cubemapファイルのパス
        //! @param  outputDirectory [in] 出力先ディレクトリ
        //! @return 成功したらtrue
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) override;
    };
}    // namespace Tsukino::Asset

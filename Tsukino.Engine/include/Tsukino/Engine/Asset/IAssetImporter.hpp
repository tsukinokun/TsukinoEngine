//--------------------------------------------------------------
//! @file	IAssetImporter.hpp
//! @brief  アセットインポーターの共通インターフェース
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Memory.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    class IAsset;    // 前方宣言
    //--------------------------------------------------------------
    //! @class IAssetImporter
    //! @brief アセットインポーターの共通インターフェース
    //--------------------------------------------------------------
    class IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! @brief 仮想デストラクタ
        //--------------------------------------------------------------
        virtual ~IAssetImporter() = default;

        //--------------------------------------------------------------
        //! @brief  アセットをインポートする関数
        //! @param  path            [in] インポートするアセットのパス
        //! @param  outPutDirectory [in] インポートされたアセットの出力先ディレクトリ
        //! @return インポートされたアセットの参照
        //--------------------------------------------------------------
        [[nodiscard]]
        virtual bool Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) = 0;
    };
}    // namespace Tsukino::Asset

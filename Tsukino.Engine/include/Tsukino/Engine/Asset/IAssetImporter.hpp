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
        //! @param  path [in] インポートするアセットのパス
        //! @return インポートされたアセットの参照
        //--------------------------------------------------------------
        virtual Tsukino::Core::Ref<IAsset> Import(const Tsukino::Core::Path& path) = 0;
    };
}    // namespace Tsukino::Asset

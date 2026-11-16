//--------------------------------------------------------------
//! @file   CubemapAsset.hpp
//! @brief  スカイボックスアセット
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAsset.hpp>
#include <vector>
#include <cstdint>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  CubemapAsset
    //! @brief  キューブマップのファイル（KTX）の中身を保持するアセット
    //--------------------------------------------------------------
    class CubemapAsset : public IAsset {
    public:
        //--------------------------------------------------------------
        //! @brief アセットの種類を取得する関数
        //--------------------------------------------------------------
        [[nodiscard]] AssetType GetType() const override { return AssetType::Cubemap; }

        // キューブマップのファイル（KTX。6面とミップを含む）の中身
        std::vector<uint8_t> data;

        // メタデータ
        uint32_t width  = 0;
        uint32_t height = 0;
    };

}    // namespace Tsukino::Asset

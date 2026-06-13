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
    //! @brief  キューブマップDDSのバイナリを保持するアセット
    //--------------------------------------------------------------
    class CubemapAsset : public IAsset {
    public:
        //--------------------------------------------------------------
        //! @brief ハンドルを取得する関数
        //--------------------------------------------------------------
        [[nodiscard]] AssetHandle GetHandle() const override { return m_handle; }

        //--------------------------------------------------------------
        //! @brief アセットの種類を取得する関数
        //--------------------------------------------------------------
        [[nodiscard]] AssetType GetType() const override { return AssetType::Cubemap; }

        //--------------------------------------------------------------
        //! @brief ハンドルを設定する関数
        //--------------------------------------------------------------
        void SetHandle(const AssetHandle& h) override { m_handle = h; }

        // キューブマップDDSのバイナリデータ
        std::vector<uint8_t> ddsData;

        // メタデータ
        uint32_t width  = 0;
        uint32_t height = 0;

    private:
        AssetHandle m_handle;
    };

}    // namespace Tsukino::Asset

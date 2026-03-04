//--------------------------------------------------------------
//! @file   TextureAsset.hpp
//! @brief  テクスチャアセットクラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAsset.hpp>

#include <dxgiformat.h>

#include <vector>
#include <cstdint>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  TextureAsset
    //! @brief  テクスチャアセット
    //--------------------------------------------------------------
    class TextureAsset : public IAsset {
    public:
        //--------------------------------------------------------------
        //! @brief ハンドルを取得する関数
        //! @return アセットのハンドル
        //--------------------------------------------------------------
        AssetHandle GetHandle() const override { return m_handle; }

        //--------------------------------------------------------------
        //! @brief アセットの種類を取得する関数
        //! @return アセットの種類
        //--------------------------------------------------------------
        AssetType GetType() const override { return AssetType::Texture; }

        //--------------------------------------------------------------
        //! @brief  ローダー側から設定されるハンドル
        //! @param handle [in] 設定するハンドル
        //--------------------------------------------------------------
        void SetHandle(AssetHandle handle) { m_handle = handle; }

        uint32_t             width  = 0;                      // テクスチャの幅
        uint32_t             height = 0;                      // テクスチャの高さ
        DXGI_FORMAT          format = DXGI_FORMAT_UNKNOWN;    // テクスチャのフォーマット
        std::vector<uint8_t> pixels;                          // テクスチャのピクセルデータ（RGBA8 形式など）

    private:
        AssetHandle m_handle = AssetHandle::Invalid();    // アセットのハンドル
    };

}    // namespace Tsukino::Asset

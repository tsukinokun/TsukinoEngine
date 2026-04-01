//--------------------------------------------------------------
//! @file       FontAsset.hpp
//! @brief      DirectXTKのSpriteFontを管理するアセットクラス
//! @author     山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAsset.hpp>

#include <memory>
#include <string>
#include <d3d11.h>
// Tsukino::Asset 名前空間
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class FontAsset
    //! @brief DirectXTKのSpriteFontを管理するアセットクラス
    //--------------------------------------------------------------
    class FontAsset : public IAsset {
    public:
        //--------------------------------------------------------------
        //! @brief ハンドルを取得する関数
        //! @return アセットのハンドル
        //--------------------------------------------------------------
        [[nodiscard]] AssetHandle GetHandle() const override { return m_handle; }

        //--------------------------------------------------------------
        //! @brief タイプを取得する関数
        //! @return アセットの種類
        //--------------------------------------------------------------
        [[nodiscard]] AssetType GetType() const override { return AssetType::Font; }

        //--------------------------------------------------------------
        //! @brief ハンドル設定用のセッター
        //! @param h [in] 設定するハンドル
        //--------------------------------------------------------------
        void SetHandle(AssetHandle h) { m_handle = h; }

        std::vector<uint8_t> m_binaryData;    // フォントデータのバイナリ

    private:
        AssetHandle m_handle;    // アセットハンドル
    };

}    // namespace Tsukino::Asset

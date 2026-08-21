//--------------------------------------------------------------
//! @file       DynamicFontAsset.hpp
//! @brief      オンデマンドラスタライズ用フォント(ttf/otf生データ)を管理するアセットクラス
//! @author     山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAsset.hpp>

#include <string>
#include <vector>
#include <cstdint>

// Tsukino::Asset 名前空間
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class DynamicFontAsset
    //! @brief 実行時にグリフをオンデマンドラスタライズするためのフォントデータを保持するアセット
    //--------------------------------------------------------------
    class DynamicFontAsset : public IAsset {
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
        [[nodiscard]] AssetType GetType() const override { return AssetType::DynamicFont; }

        //--------------------------------------------------------------
        //! @brief ハンドル設定用のセッター
        //! @param h [in] 設定するハンドル
        //--------------------------------------------------------------
        void SetHandle(const AssetHandle& h) override { m_handle = h; }

        std::string           m_faceName;      // フォントの表示名(デバッグ用)
        float                 m_pixelSize = 0;  // グリフをラスタライズする基準ピクセルサイズ
        std::vector<uint8_t>  m_fontFileData;   // ttf/otfの生データ

    private:
        AssetHandle m_handle;    // アセットハンドル
    };

}    // namespace Tsukino::Asset

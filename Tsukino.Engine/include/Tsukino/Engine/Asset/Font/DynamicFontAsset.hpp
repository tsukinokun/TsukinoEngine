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
        //! @brief タイプを取得する関数
        //! @return アセットの種類
        //--------------------------------------------------------------
        [[nodiscard]] AssetType GetType() const override { return AssetType::DynamicFont; }

        std::string           faceName;      // フォントの表示名(デバッグ用)
        float                 pixelSize = 0;  // グリフをラスタライズする基準ピクセルサイズ
        std::vector<uint8_t>  fontFileData;   // ttf/otfの生データ
    };

}    // namespace Tsukino::Asset

//--------------------------------------------------------------
//! @file       FontAsset.hpp
//! @brief      ベイク済みフォント（.spritefont）のバイナリを持つアセットクラス
//! @author     山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAsset.hpp>

#include <cstdint>
#include <vector>

// Tsukino::Asset 名前空間
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class FontAsset
    //! @brief ベイク済みフォントのバイナリを持つアセットクラス（GPU の資源は Renderer 側が作る）
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
        void SetHandle(const AssetHandle& h) override { m_handle = h; }

        std::vector<uint8_t> m_binaryData;    // フォントデータのバイナリ

    private:
        AssetHandle m_handle;    // アセットハンドル
    };

}    // namespace Tsukino::Asset

//--------------------------------------------------------------
//! @file   EffectAsset.hpp
//! @brief  エフェクトアセットクラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAsset.hpp>

#include <vector>
#include <cstdint>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  EffectAsset
    //! @brief  .efk エフェクトファイルのアセット
    //--------------------------------------------------------------
    class EffectAsset : public IAsset {
    public:
        //--------------------------------------------------------------
        //! @brief ハンドルを取得する関数
        //! @return アセットのハンドル
        //--------------------------------------------------------------
        [[nodiscard]]
        AssetHandle GetHandle() const override {
            return m_handle;
        }

        //--------------------------------------------------------------
        //! @brief アセットの種類を取得
        //! @return アセットの種類
        //--------------------------------------------------------------
        [[nodiscard]]
        AssetType GetType() const override {
            return AssetType::Effect;
        }

        //--------------------------------------------------------------
        //! @brief  ローダー側から設定されるハンドル
        //! @param handle [in] 設定するハンドル
        //--------------------------------------------------------------
        void SetHandle(const AssetHandle& h) { m_handle = h; }

        std::vector<uint8_t> binary;    // .efk ファイルのバイナリデータ

    private:
        AssetHandle m_handle = AssetHandle::Invalid();    // アセットのハンドル
    };
}

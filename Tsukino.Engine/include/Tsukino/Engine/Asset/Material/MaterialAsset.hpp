//--------------------------------------------------------------
//! @file   MaterialAsset.hpp
//! @brief  マテリアルアセットの定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAsset.hpp>
#include <Tsukino/GraphicsCommon/Material/MaterialData.hpp>
#include <hlsl++.h>
// 名前空間 ：Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  MaterialAsset
    //! @brief  マテリアルアセットクラス
    //--------------------------------------------------------------
    class MaterialAsset : public IAsset {
    public:
        //--------------------------------------------------------------
        //! @brief  ハンドルを取得する関数
        //! @return アセットのハンドル
        //--------------------------------------------------------------
        [[nodiscard]]
        AssetHandle GetHandle() const override {
            return m_handle;
        }

        //--------------------------------------------------------------
        //! @brief  アセットの種類を取得する関数
        //! @return アセットの種類
        //--------------------------------------------------------------
        [[nodiscard]]
        AssetType GetType() const override {
            return AssetType::Material;
        }

        //--------------------------------------------------------------
        //! @brief ハンドル設定用のセッター
        //! @param h [in] 設定するハンドル
        //--------------------------------------------------------------
        void SetHandle(const AssetHandle& h) { m_handle = h; }

        // PBRパラメータはMaterialDataに任せる
        Tsukino::GraphicsCommon::MaterialData data;

        //--------------------------------------------------------------
        //! @brief テクスチャハンドル（ランタイム専用）
        //! @note  各ハンドルのバインド先スロットはShaderSlots.hppを参照
        //!        SRVSlot::Albedo            = t0
        //!        SRVSlot::Normal            = t1
        //!        SRVSlot::MetallicRoughness = t2
        //!        SRVSlot::Emissive          = t3
        //!        SRVSlot::AO                = t4
        //--------------------------------------------------------------
        AssetHandle albedoHandle;               // アルベドテクスチャのハンドル
        AssetHandle normalHandle;               // 法線テクスチャのハンドル
        AssetHandle metallicRoughnessHandle;    // メタリック・ラフネステクスチャのハンドル
        AssetHandle emissiveHandle;             // エミッシブテクスチャのハンドル
        AssetHandle aoHandle;                   // アンビエントオクルージョンテクスチャのハンドル

        AssetHandle vertexShaderHandle;    // VS
        AssetHandle pixelShaderHandle;     // PS

    private:
        AssetHandle m_handle;    // IAssetの実体となる識別子
    };
}    // namespace Tsukino::Asset

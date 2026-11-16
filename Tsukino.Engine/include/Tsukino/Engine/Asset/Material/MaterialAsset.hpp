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
        //! @brief  アセットの種類を取得する関数
        //! @return アセットの種類
        //--------------------------------------------------------------
        [[nodiscard]]
        AssetType GetType() const override {
            return AssetType::Material;
        }

        // PBRパラメータはMaterialDataに任せる
        Tsukino::GraphicsCommon::MaterialData data;

        //--------------------------------------------------------------
        //! @brief テクスチャハンドル（ランタイム専用）
        //! @note  各ハンドルは Renderer の MaterialTextureSlot の同じ名前の枠へ渡す
        //--------------------------------------------------------------
        AssetHandle albedoHandle;               // アルベドテクスチャのハンドル
        AssetHandle normalHandle;               // 法線テクスチャのハンドル
        AssetHandle metallicRoughnessHandle;    // メタリック・ラフネステクスチャのハンドル
        AssetHandle emissiveHandle;             // エミッシブテクスチャのハンドル
        AssetHandle aoHandle;                   // アンビエントオクルージョンテクスチャのハンドル

        AssetHandle vertexShaderHandle;    // VS
        AssetHandle pixelShaderHandle;     // PS
    };
}    // namespace Tsukino::Asset

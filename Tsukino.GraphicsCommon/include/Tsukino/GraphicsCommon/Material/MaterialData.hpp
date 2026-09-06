//--------------------------------------------------------------
//! @file   Material.hpp
//! @brief  マテリアルデータの構造体を定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>
#include <hlsl++.h>
#include <string>

// 名前空間 Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {

    //--------------------------------------------------------------
    //! @enum ShadingModel
    //! @brief シェーディングモデルの種類
    //--------------------------------------------------------------
    enum class ShadingModel : u8 {
        PBR,      // 物理ベースレンダリング
        Unlit,    // ライティングなし
        Toon,     // トゥーンシェーディング
        Water,    // 水面
    };

    //--------------------------------------------------------------
    //! @struct MaterialData
    //! @brief  マテリアルデータの構造体
    //--------------------------------------------------------------
    struct MaterialData {
        std::string  name;
        ShadingModel shadingModel = ShadingModel::PBR;

        //--------------------------------------------------------------
        // PBR パラメータ（テクスチャがない場合のフォールバック値）
        //--------------------------------------------------------------
        hlslpp::interop::float4 baseColor = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);
        hlslpp::interop::float3 emissive  = hlslpp::float3(0.0f, 0.0f, 0.0f);
        float                   metallic  = 0.0f;
        float                   roughness = 0.5f;
        float                   specular  = 0.5f;

        //--------------------------------------------------------------
        // アルファテスト（カットアウト）のしきい値
        // 0 = 無効。0より大きいとき、アルベドテクスチャのアルファがこの値未満の
        // テクセルをピクセルシェーダーが破棄する。
        // ModelImporterがアルベドの透明テクセルを検出して自動で設定する
        //--------------------------------------------------------------
        float alphaCutoff = 0.0f;

        //--------------------------------------------------------------
        // Water シェーディング用パラメータ
        //--------------------------------------------------------------
        hlslpp::interop::float2 waterSpeed  = hlslpp::float2(0.1f, 0.1f);
        float                   waterScale  = 1.0f;
        float                   waterHeight = 0.05f;

        // テクスチャパス（空文字 = 未使用）
        std::string albedoMap;
        std::string normalMap;
        std::string metallicRoughnessMap;
        std::string emissiveMap;
        std::string aoMap;

        //--------------------------------------------------------------
        //! @brief cereal シリアライズ
        //--------------------------------------------------------------
        template <class Archive>
        void serialize(Archive& ar) {
            ar(name, shadingModel, baseColor, emissive, metallic, roughness, specular, alphaCutoff, albedoMap, normalMap, metallicRoughnessMap, emissiveMap, aoMap);
        }
    };

}    // namespace Tsukino::GraphicsCommon

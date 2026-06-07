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
    };

    //--------------------------------------------------------------
    //! @struct MaterialData
    //! @brief  マテリアルデータの構造体
    //--------------------------------------------------------------
    struct MaterialData {
        std::string  name;
        ShadingModel shadingModel = ShadingModel::PBR;

        // PBR パラメータ（テクスチャがない場合のフォールバック値）
        hlslpp::interop::float4 baseColor = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);
        hlslpp::interop::float3 emissive  = hlslpp::float3(0.0f, 0.0f, 0.0f);
        float                   metallic  = 0.0f;
        float                   roughness = 0.5f;
        float                   specular  = 0.5f;

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
            ar(name, shadingModel, baseColor, emissive, metallic, roughness, specular, albedoMap, normalMap, metallicRoughnessMap, emissiveMap, aoMap);
        }
    };

}    // namespace Tsukino::GraphicsCommon

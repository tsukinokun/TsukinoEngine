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
        // トゥーンのパラメータ（shadingModel が Toon のときだけ使う）。
        // 光の当たり具合（光の真裏 0・真横 0.5・正面 1）が threshold を超えた所を明るい側、下回った所を暗い側にし、
        // 境目を smoothness の幅でぼかす。影の中も暗い側になる。暗い側は明るい側の色に toonShadeColor を掛けた色になる
        //--------------------------------------------------------------
        float                   toonThreshold    = 0.5f;                                   // 明るい側と暗い側の境目（0〜1）
        float                   toonSmoothness   = 0.05f;                                  // 境目のぼかし幅（0〜1。0 でくっきり）
        hlslpp::interop::float3 toonShadeColor   = hlslpp::float3(0.5f, 0.5f, 0.5f);    // 暗い側に掛ける色（0〜1）
        float                   toonSpecularSize = 0.0f;                                   // くっきりしたハイライトの大きさ（0〜1。0 で無し）

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
            ar(name, shadingModel, baseColor, emissive, metallic, roughness, specular, alphaCutoff, toonThreshold, toonSmoothness, toonShadeColor, toonSpecularSize,
               albedoMap, normalMap, metallicRoughnessMap, emissiveMap, aoMap);
        }
    };

}    // namespace Tsukino::GraphicsCommon

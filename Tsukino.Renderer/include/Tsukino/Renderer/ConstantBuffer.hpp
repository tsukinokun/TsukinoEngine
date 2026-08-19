//--------------------------------------------------------------
//! @file   ConstantBuffer.hpp
//! @brief  VS用定数バッファ構造体（行列）
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
#include <hlsl++.h>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @struct CBufferScene
    //! @brief  スロット0 (b0) 用：フレーム内で全オブジェクト共通のデータ
    //--------------------------------------------------------------
    struct CBufferScene {
        Tsukino::Core::Math::matrix view;
        Tsukino::Core::Math::matrix projection;
        Tsukino::Core::Math::matrix viewProj;
        Tsukino::Core::Math::matrix invViewProj;      //!< viewProjの逆行列（スカイ・ポストエフェクト等で使用）
        Tsukino::Core::Math::matrix lightViewProj;    //!< ライト空間のViewProjection行列
        hlslpp::float4              lightDir;         //!< ライトの方向
        hlslpp::float4              lightColor;       //!< ライト色と強度 xyz: 色(linear), w: 強度
        hlslpp::float4              cameraPos;        //!< カメラのワールド座標 xyz: 座標, w: 未使用
    };

    //--------------------------------------------------------------
    //! @struct CBufferTransform
    //! @brief  スロット1 (b1) 用：オブジェクトごとの固有データ
    //--------------------------------------------------------------
    struct CBufferTransform {
        Tsukino::Core::Math::matrix world;
    };

    //--------------------------------------------------------------
    //! @struct CBufferMaterial
    //! @brief  スロット2 (b2) 用：マテリアルごとの固有データ
    //--------------------------------------------------------------
    struct CBufferMaterial {
        hlslpp::float4 baseColor;
        hlslpp::float3 emissive;
        float          metallic;
        float          roughness;
        float          specular;
        hlslpp::float4 rimColor;     //!< xyz: ふちの色, w: ふちの強さ（旧paddingを転用。ハイライト演出用）
        hlslpp::float4 rimParams;    //!< x: ふちの鋭さ(pow指数), y: 全体の白発光量, zw: 予約
    };

    //--------------------------------------------------------------
    //! @struct CBufferSkinning
    //! @brief  スロット3 (b3) 用：アニメーションするオブジェクトのボーン行列
    //--------------------------------------------------------------
    struct CBufferSkinning {
        hlslpp::float4x4 bones[128];    // hlslpp::float4x4 の配列（最大128本分）
    };

    //--------------------------------------------------------------
    //! @struct CBufferSky
    //! @brief  スロット4 (b4) 用：大気散乱パラメータ
    //--------------------------------------------------------------
    struct CBufferSky {
        //----------------------------------------------------------
        // 散乱パラメータ
        //----------------------------------------------------------
        float rayleighScattering;    //!< レイリー散乱の強さ
        float mieScattering;         //!< ミー散乱の強さ
        float mieAnisotropy;         //!< ミー散乱の異方性
        float sunIntensity;          //!< 太陽の強度

        //----------------------------------------------------------
        // 大気パラメータ
        //----------------------------------------------------------
        float atmosphereHeight;    //!< 大気の厚さ
        float planetRadius;        //!< 地球の半径
        float sunDiskSize;         //!< 太陽円盤の大きさ
        float padding0;            //!< 16バイトアライメント用

        //----------------------------------------------------------
        // 地面カラー・太陽方向
        //----------------------------------------------------------
        hlslpp::float4 groundColor;     //!< xyz: 地面カラー, w: 未使用
        hlslpp::float4 sunDirection;    //!< xyz: 太陽方向（正規化済み）, w: 未使用
    };

    //--------------------------------------------------------------
    //! @struct CBufferWater
    //! @brief  スロット5 (b5) 用：水面パラメータ
    //--------------------------------------------------------------
    struct CBufferWater {
        float time;            //!< 経過時間（UVスクロール用）
        float waveSpeed;       //!< 波のスクロール速度
        float waveScale;       //!< 波のUVスケール
        float fresnelPower;    //!< フレネルの強さ

        hlslpp::float4 shallowColor;    //!< 浅瀬の色 xyz: 色, w: 未使用
        hlslpp::float4 deepColor;       //!< 深部の色 xyz: 色, w: 未使用
    };

}    // namespace Tsukino::Renderer

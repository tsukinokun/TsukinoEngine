//--------------------------------------------------------------
//! @file   IBL.hlsli
//! @brief  スカイ由来IBL（image-based lighting）の共通定義
//! @author 山﨑愛
//! @note   フォワード(Model.ps.hlsl)とディファード(Lighting.ps.hlsl)で
//!         アンビエント項の式が乖離しないよう、ここに一元化する
//!         （PBR.hlsliが直接照明のBRDFを一元化しているのと同じ考え方）。
//--------------------------------------------------------------
#ifndef TSUKINO_IBL_HLSLI
#define TSUKINO_IBL_HLSLI

#include "PBR.hlsli"

//--------------------------------------------------------------
//! @brief IBLパラメータ (b10)
//--------------------------------------------------------------
cbuffer CBufferIBL : register(b10)
{
    float specularMipCount;    // プレフィルタ済みスペキュラキューブマップのミップ数
    float iblIntensity;        // IBL全体の強度倍率
    float2 iblPad;             // 予約
};

//--------------------------------------------------------------
//! @brief IBLテクスチャ (t17〜t19) とサンプラー (s10)
//--------------------------------------------------------------
TextureCube iblIrradianceMap : register(t17);            // 拡散IBL：/PI済みのirradiance
TextureCube iblPrefilteredSpecularMap : register(t18);    // 鏡面IBL：ミップ=ラフネスのプレフィルタ済み放射輝度
Texture2D   iblBRDFLUT : register(t19);                   // split-sum用BRDF積分LUT（r: スケールA, g: バイアスB）
SamplerState iblSampler : register(s10);

//--------------------------------------------------------------
//! @brief スカイ由来IBLのアンビエント項を評価する
//! @param N         法線（正規化済み）
//! @param V         視線ベクトル（ピクセル→カメラ、正規化済み）
//! @param albedo    アルベド色
//! @param metallic  メタリック [0,1]
//! @param roughness ラフネス [0,1]
//! @param specular  非金属のスペキュラ強度
//! @param ao        アンビエントオクルージョン [0,1]（未対応パスは1.0を渡す）
//! @return 拡散+鏡面のIBL寄与（そのままfinalColorへ加算する）
//--------------------------------------------------------------
float3 EvaluateIBL(float3 N, float3 V, float3 albedo, float metallic, float roughness, float specular, float ao)
{
    float NdotV = saturate(dot(N, V)) + 1e-5f;

    // F0: 垂直入射時の反射率（誘電体はspecularから、金属はアルベドから。EvaluatePBRと同じ式）
    float3 F0 = lerp(0.08f * specular.xxx, albedo, metallic);

    // 環境光用フレネル。ラフネスが高いほど grazing angle での白浮きを抑える
    // （Karis "Real Shading in Unreal Engine 4" のroughness項付きSchlick近似）
    float3 F = F0 + (max((1.0f - roughness).xxx, F0) - F0) * pow(saturate(1.0f - NdotV), 5.0f);

    float3 kD = (1.0f - F) * (1.0f - metallic);

    //----------------------------------------------------------
    // 拡散IBL：irradianceは畳み込み側で既に/PI・コサイン重み積分済みなので、
    // ここではalbedoを掛けるだけでよい
    //----------------------------------------------------------
    float3 irradiance = iblIrradianceMap.SampleLevel(iblSampler, N, 0.0f).rgb;
    float3 diffuseIBL = kD * albedo * irradiance;

    //----------------------------------------------------------
    // 鏡面IBL：split-sum近似。プレフィルタ済み放射輝度 × (F0*スケール + バイアス)
    //----------------------------------------------------------
    float3 R   = reflect(-V, N);
    float  lod = roughness * max(specularMipCount - 1.0f, 0.0f);

    float3 prefilteredColor = iblPrefilteredSpecularMap.SampleLevel(iblSampler, R, lod).rgb;
    float2 envBRDF          = iblBRDFLUT.SampleLevel(iblSampler, float2(NdotV, roughness), 0.0f).rg;
    float3 specularIBL      = prefilteredColor * (F0 * envBRDF.x + envBRDF.y);

    return (diffuseIBL + specularIBL) * ao * iblIntensity;
}

#endif    // TSUKINO_IBL_HLSLI

//--------------------------------------------------------------
//! @file   Lighting.hlsli
//! @brief  ディファードLightingパス用の共通定義
//!         点光源・スポットライト配列、G-Bufferの入力、
//!         シャドウPCFサンプリングを提供する。
//! @author 山﨑愛
//--------------------------------------------------------------
#ifndef TSUKINO_LIGHTING_HLSLI
#define TSUKINO_LIGHTING_HLSLI

#include "PBR.hlsli"

//--------------------------------------------------------------
//! @brief 点光源・スポットライト1灯分のGPUデータ (64B)
//--------------------------------------------------------------
struct GPULight
{
    float4 positionRange;     // xyz: ワールド座標, w: 影響半径
    float4 colorIntensity;    // xyz: 色(linear), w: 強度
    float4 directionType;     // xyz: 方向（スポットのみ有効）, w: 0=Point, 1=Spot
    float4 spotParams;        // x: cos(内側角), y: cos(外側角), zw: 予約
};

#define TSUKINO_MAX_LIGHTS 64

//--------------------------------------------------------------
//! @brief 点光源・スポットライト配列 (b5)
//--------------------------------------------------------------
cbuffer CBufferLights : register(b5)
{
    uint     lightCount;
    uint3    lightsPad;
    GPULight lights[TSUKINO_MAX_LIGHTS];
};

//--------------------------------------------------------------
//! @brief G-Buffer (t9〜t12)、深度 (t13)、ワールド座標 (t14)
//! @note  ワールド座標は深度からの再構成ではなく、GBufferパスの頂点シェーダー
//!        補間値をそのまま出力したものを読む（フォワードと同じ精度を保証するため）。
//--------------------------------------------------------------
Texture2D gbufferAlbedo : register(t9);
Texture2D gbufferNormal : register(t10);
Texture2D gbufferMaterial : register(t11);
Texture2D gbufferEmissive : register(t12);
Texture2D gbufferDepth : register(t13);
Texture2D gbufferWorldPos : register(t14);

//--------------------------------------------------------------
//! @brief G-Buffer/深度読み取り用ポイントサンプラー (s9)
//--------------------------------------------------------------
SamplerState gbufferSampler : register(s9);

//--------------------------------------------------------------
//! @brief シャドウマップ (t8) と比較サンプラー (s8)
//--------------------------------------------------------------
Texture2D               shadowMap : register(t8);
SamplerComparisonState  shadowSampler : register(s8);

//--------------------------------------------------------------
//! @brief PCFシャドウサンプリング（式はPBR.hlsliのSampleShadowPCFに一元化してある）
//! @return 遮蔽量 0.0f(暗) - 1.0f(明)
//--------------------------------------------------------------
float GetShadowPCF(float3 worldPos, float3 N, float3 L)
{
    return SampleShadowPCF(shadowMap, shadowSampler, worldPos, N, L);
}

//--------------------------------------------------------------
//! @brief 点光源・スポットライトの距離減衰（逆二乗 + rangeでのスムーズカットオフ）
//--------------------------------------------------------------
float Attenuate(float dist, float range)
{
    float d2          = dist * dist;
    float rangeFactor = saturate(1.0f - pow(dist / max(range, 1e-4f), 4.0f));
    return (rangeFactor * rangeFactor) / (d2 + 1.0f);
}

#endif    // TSUKINO_LIGHTING_HLSLI

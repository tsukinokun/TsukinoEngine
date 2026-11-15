//--------------------------------------------------------------
//! @file   Lighting.sh
//! @brief  ディファードライティングの入力
//! @note   点光源・スポットライトの定数（C++ の CBufferLights）は vec4 の配列 u_lights。
//!         [0].x が光の数（uint のビットのまま届くので floatBitsToUint で読む）、[1 + 4i] から1つの光が4つずつ並ぶ
//--------------------------------------------------------------
#ifndef TSUKINO_LIGHTING_SH
#define TSUKINO_LIGHTING_SH

#include "PBR.sh"
#include "TextureStages.sh"

#define TSUKINO_MAX_LIGHTS 64
#define TSUKINO_LIGHTS_VEC4_COUNT 257    // sizeof(CBufferLights) / 16

uniform vec4 u_lights[TSUKINO_LIGHTS_VEC4_COUNT];

#define lightCount              int(floatBitsToUint(u_lights[0].x))
#define lightPositionRange(_i)  u_lights[1 + (_i) * 4]    // xyz: ワールド座標, w: 影響半径
#define lightColorIntensity(_i) u_lights[2 + (_i) * 4]    // xyz: 色(linear), w: 強度
#define lightDirectionType(_i)  u_lights[3 + (_i) * 4]    // xyz: 方向（スポットのみ）, w: 0=Point, 1=Spot
#define lightSpotParams(_i)     u_lights[4 + (_i) * 4]    // x: cos(内側角), y: cos(外側角)

SAMPLER2D(s_gbufferAlbedo, STAGE_GBUFFER_ALBEDO);
SAMPLER2D(s_gbufferNormal, STAGE_GBUFFER_NORMAL);
SAMPLER2D(s_gbufferMaterial, STAGE_GBUFFER_MATERIAL);
SAMPLER2D(s_gbufferEmissive, STAGE_GBUFFER_EMISSIVE);
SAMPLER2D(s_gbufferDepth, STAGE_GBUFFER_DEPTH);
SAMPLER2D(s_gbufferWorldPos, STAGE_GBUFFER_WORLD_POS);

//--------------------------------------------------------------
//! @brief  太陽の影を読む（式は PBR.sh の SampleShadowPCF に一元化してある）
//! @return 遮られていない割合 0.0(影) - 1.0(明)
//--------------------------------------------------------------
float GetShadowPCF(vec3 worldPos, vec3 N, vec3 L)
{
    return SampleShadowPCF(worldPos, N, L);
}

//--------------------------------------------------------------
//! @brief  点光源の距離による減衰
//--------------------------------------------------------------
float Attenuate(float dist, float range)
{
    float d2          = dist * dist;
    float rangeFactor = saturate(1.0 - pow(dist / max(range, 1e-4), 4.0));
    return (rangeFactor * rangeFactor) / (d2 + 1.0);
}

#endif    // TSUKINO_LIGHTING_SH

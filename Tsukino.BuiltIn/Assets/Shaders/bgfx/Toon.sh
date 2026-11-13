//--------------------------------------------------------------
//! @file   Toon.sh
//! @brief  ShadingModel の番号とトゥーンのライティング（Toon.hlsli の bgfx 版）
//--------------------------------------------------------------
#ifndef TSUKINO_TOON_SH
#define TSUKINO_TOON_SH

#include "PBR.sh"

#define SHADING_MODEL_PBR   0
#define SHADING_MODEL_UNLIT 1
#define SHADING_MODEL_TOON  2

//--------------------------------------------------------------
//! @brief  ShadingModel の番号を G-Buffer の 0〜1 の値にする / 戻す
//--------------------------------------------------------------
float EncodeShadingModel(int id)
{
    return float(id) / 3.0;
}

int DecodeShadingModel(float enc)
{
    return int(floor(saturate(enc) * 3.0 + 0.5));
}

//--------------------------------------------------------------
//! @brief  トゥーンの暗い側の色を float 1つに詰める / 戻す（8bit × 3）
//--------------------------------------------------------------
float PackShadeColor(vec3 color)
{
    vec3 c = floor(saturate(color) * 255.0 + 0.5);
    return c.r + c.g * 256.0 + c.b * 65536.0;
}

vec3 UnpackShadeColor(float packed)
{
    float v = floor(packed + 0.5);
    float b = floor(v / 65536.0);
    float g = floor((v - b * 65536.0) / 256.0);
    float r = v - b * 65536.0 - g * 256.0;
    return vec3(r, g, b) / 255.0;
}

//--------------------------------------------------------------
//! @brief  トゥーンの段差（境目を softness の幅でなめらかにする）
//--------------------------------------------------------------
float ToonStep(float x, float threshold, float softness)
{
    float w = max(softness, 1e-3);    // 0 だと smoothstep の幅が 0 になり NaN を出すので下限を付ける
    return smoothstep(threshold - w, threshold + w, x);
}

//--------------------------------------------------------------
//! @brief  1つの光をトゥーンで当てる（Toon.hlsli と同じ）
//--------------------------------------------------------------
vec3 EvaluateToon(vec3 N, vec3 V, vec3 L, vec3 albedo, vec3 radiance, float shadow, float threshold, float softness, vec3 shadeColor,
                  float specularSize, bool shadeFloor)
{
    float NdotL = dot(N, L);
    float lit   = ToonStep(NdotL * 0.5 + 0.5, threshold, softness) * ToonStep(shadow, 0.5, softness);

    vec3 bright = albedo / PI * radiance;
    vec3 dark   = shadeFloor ? bright * shadeColor : vec3_splat(0.0);
    vec3 color  = mix(dark, bright, vec3_splat(lit));

    if(specularSize > 0.0) {
        vec3  H         = normalize(V + L);
        float NdotH     = saturate(dot(N, H));
        float highlight = ToonStep(NdotH, 1.0 - specularSize * 0.1, softness * 0.1) * lit;
        color += radiance / PI * highlight;
    }
    return color;
}

#endif    // TSUKINO_TOON_SH

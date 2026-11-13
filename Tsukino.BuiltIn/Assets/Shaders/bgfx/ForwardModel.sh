//--------------------------------------------------------------
//! @file   ForwardModel.sh
//! @brief  モデルをフォワードで照らす処理（ForwardModel.hlsli の bgfx 版）
//! @note   半透明のモデル（フェード中）と、UI の層のモデルが使う。
//!         影（段階 1-4 c）と霧（段階 1-4 d）はまだ無い。影はいつも照らされている扱い
//--------------------------------------------------------------
#ifndef TSUKINO_FORWARD_MODEL_SH
#define TSUKINO_FORWARD_MODEL_SH

#include "Scene.sh"
#include "Material.sh"
#include "PBR.sh"
#include "IBL.sh"
#include "Toon.sh"
#include "TextureStages.sh"

SAMPLER2D(s_albedo, STAGE_ALBEDO);

//--------------------------------------------------------------
//! @brief  モデルの1画素をフォワードで照らした色（HDR）と不透明度を求める
//! @param  worldPos [in] ワールド座標
//! @param  normal   [in] 法線（補間されたまま）
//! @param  uv       [in] UV
//! @return rgb: 色（HDR）, a: 不透明度
//--------------------------------------------------------------
vec4 ShadeForwardModel(vec3 worldPos, vec3 normal, vec2 uv)
{
    vec4 albedoSample = texture2D(s_albedo, uv);
    if(albedoSample.a - alphaCutoff < 0.0)
        discard;

    vec3 albedo = albedoSample.rgb * baseColor.rgb;
    vec3 N      = normalize(normal);
    vec3 V      = normalize(cameraPos.xyz - worldPos);
    vec3 L      = normalize(-lightDir.xyz);    // lightDir は「光が向いている方向」なので反転

    float shadow = 1.0;    // 影は段階 1-4 c で移す

    int  shadingModel = int(floor(shading.x + 0.5));
    vec3 directLight  = vec3_splat(0.0);
    vec3 ambient      = vec3_splat(0.0);

    if(shadingModel == SHADING_MODEL_UNLIT) {
        directLight = albedo;
    } else if(shadingModel == SHADING_MODEL_TOON) {
        directLight = EvaluateToon(N, V, L, albedo, lightColor.rgb * lightColor.w, shadow, shading.y, shading.z, toonShadeColor.rgb, shading.w, true);
        ambient     = EvaluateIBL(N, V, albedo, 0.0, 1.0, 0.0, 1.0);
    } else {
        shadow        = max(shadow, kShadowMinLit);
        vec3 radiance = lightColor.rgb * lightColor.w * shadow;
        directLight   = EvaluatePBR(N, V, L, albedo, metallic, roughness, specular, radiance);
        ambient       = EvaluateIBL(N, V, albedo, metallic, roughness, specular, 1.0);
    }

    vec3 finalColor = ambient + directLight + emissive;
    finalColor += EvaluateEmissiveBoost(N, V);

    return vec4(finalColor, baseColor.a * albedoSample.a);
}

#endif    // TSUKINO_FORWARD_MODEL_SH

//--------------------------------------------------------------
//! @file   IBL.sh
//! @brief  スカイ由来の環境光（IBL.hlsli の bgfx 版）
//! @note   定数（DX11 版の b10。C++ の CBufferIBL）は u_ibl の1つの vec4。
//!         x: スペキュラのミップ数, y: 強さ
//--------------------------------------------------------------
#ifndef TSUKINO_IBL_SH
#define TSUKINO_IBL_SH

#include "PBR.sh"
#include "TextureStages.sh"

uniform vec4 u_ibl;
#define specularMipCount u_ibl.x
#define iblIntensity     u_ibl.y

SAMPLERCUBE(s_iblIrradiance, STAGE_IBL_IRRADIANCE);    // 拡散：/PI 済みの irradiance
SAMPLERCUBE(s_iblSpecular, STAGE_IBL_SPECULAR);        // 鏡面：ミップ = ラフネスのプレフィルタ済み
SAMPLER2D(s_iblBrdfLut, STAGE_IBL_BRDF_LUT);           // split-sum の BRDF 積分（r: スケール, g: バイアス）

//--------------------------------------------------------------
//! @brief  環境光（拡散 + 鏡面）を求める（IBL.hlsli と同じ式）
//--------------------------------------------------------------
vec3 EvaluateIBL(vec3 N, vec3 V, vec3 albedo, float metal, float rough, float spec, float ao)
{
    float NdotV = saturate(dot(N, V)) + 1e-5;

    vec3 F0 = mix(vec3_splat(0.08 * spec), albedo, vec3_splat(metal));
    vec3 F  = F0 + (max(vec3_splat(1.0 - rough), F0) - F0) * pow(saturate(1.0 - NdotV), 5.0);
    vec3 kD = (vec3_splat(1.0) - F) * (1.0 - metal);

    vec3 irradiance = textureCubeLod(s_iblIrradiance, N, 0.0).rgb;
    vec3 diffuseIBL = kD * albedo * irradiance;

    vec3  R   = reflect(-V, N);
    float lod = rough * max(specularMipCount - 1.0, 0.0);

    vec3 prefilteredColor = textureCubeLod(s_iblSpecular, R, lod).rgb;
    vec2 envBRDF          = texture2DLod(s_iblBrdfLut, vec2(NdotV, rough), 0.0).rg;
    vec3 specularIBL      = prefilteredColor * (F0 * envBRDF.x + envBRDF.y);

    return (diffuseIBL + specularIBL) * ao * iblIntensity;
}

#endif    // TSUKINO_IBL_SH

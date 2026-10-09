//--------------------------------------------------------------
//! @file   Toon.hlsli
//! @brief  ShadingModel（PBR / Unlit / Toon）の番号と、トゥーンの照らし方の共通定義
//! @note   フォワード(ForwardModel.hlsli)とディファード(GBuffer.ps.hlsl / Lighting.ps.hlsl)で
//!         式と G-Buffer への詰め方が乖離しないよう、ここに一元化する。
//!         番号は C++ の Tsukino::GraphicsCommon::ShadingModel の並びと一致させる。
//--------------------------------------------------------------
#ifndef TSUKINO_TOON_HLSLI
#define TSUKINO_TOON_HLSLI

#include "PBR.hlsli"

//--------------------------------------------------------------
// ShadingModel の番号（ShadingModel の並びと同じ）
//--------------------------------------------------------------
static const uint SHADING_MODEL_PBR   = 0;
static const uint SHADING_MODEL_UNLIT = 1;
static const uint SHADING_MODEL_TOON  = 2;

//--------------------------------------------------------------
//! @brief ShadingModel の番号を G-Buffer1 のアルファ（2bit UNORM）へ詰める
//! @param id ShadingModel の番号（0〜3）
//--------------------------------------------------------------
float EncodeShadingModel(uint id)
{
    return (float)id / 3.0f;
}

//--------------------------------------------------------------
//! @brief G-Buffer1 のアルファから ShadingModel の番号を戻す
//--------------------------------------------------------------
uint DecodeShadingModel(float enc)
{
    return (uint)round(saturate(enc) * 3.0f);
}

//--------------------------------------------------------------
//! @brief トゥーンの暗い側の色（0〜1 の RGB）を 32bit 浮動小数の1値へ詰める
//! @note  8bit × 3 の整数（最大 2^24 - 1）にする。float の仮数は 24bit なので
//!        この範囲の整数はそのまま正確に入り、G-Buffer4 の空いているアルファに置ける
//--------------------------------------------------------------
float PackShadeColor(float3 color)
{
    const uint3 c = (uint3)round(saturate(color) * 255.0f);
    return (float)(c.r + c.g * 256 + c.b * 65536);
}

//--------------------------------------------------------------
//! @brief PackShadeColor で詰めた値から色を戻す
//--------------------------------------------------------------
float3 UnpackShadeColor(float packed)
{
    const uint v = (uint)round(packed);
    return float3(v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF) / 255.0f;
}

//--------------------------------------------------------------
//! @brief トゥーンの段階（0=暗い側, 1=明るい側）を求める
//! @param x         段階にする値（光の当たり具合など。0〜1）
//! @param threshold 境目
//! @param softness  境目のぼかし幅（0 でくっきり）
//--------------------------------------------------------------
float ToonStep(float x, float threshold, float softness)
{
    const float w = max(softness, 1e-3f);    // 0 だと smoothstep の幅が 0 になり NaN を出すので下限を付ける
    return smoothstep(threshold - w, threshold + w, x);
}

//--------------------------------------------------------------
//! @brief トゥーンで1灯分の直接照明を求める
//! @param N            法線（正規化済み）
//! @param V            視線ベクトル（ピクセル→カメラ、正規化済み）
//! @param L            ライトベクトル（ピクセル→ライト、正規化済み）
//! @param albedo       アルベド色
//! @param radiance     ライトの放射輝度（色 × 強度 × 減衰。影は含めない）
//! @param shadow       影の係数（0=影, 1=日向）。影も同じ境目で段階にする
//! @param threshold    明るい側と暗い側の境目
//! @param softness     境目のぼかし幅
//! @param shadeColor   暗い側に掛ける色
//! @param specularSize くっきりしたハイライトの大きさ（0 で無し）
//! @param shadeFloor   true なら暗い側にも shadeColor の分だけ光を残す（主光源用）。
//!                     false なら暗い側は光を足さない（点光源など、足し合わせる光用）
//! @return 直接照明の寄与
//! @note   明るさは EvaluatePBR の拡散（albedo / PI × NdotL）と揃え、
//!         PBR の物と並べても明るさが浮かないようにしている
//--------------------------------------------------------------
float3 EvaluateToon(float3 N, float3 V, float3 L, float3 albedo, float3 radiance, float shadow, float threshold, float softness, float3 shadeColor,
                    float specularSize, bool shadeFloor)
{
    const float NdotL = dot(N, L);

    // 光の当たり具合（-1〜1）を 0〜1 にしてから段階にする。影も同じ境目で切る
    const float lit = ToonStep(NdotL * 0.5f + 0.5f, threshold, softness) * ToonStep(shadow, 0.5f, softness);

    // 明るい側は正面から光を受けたときの明るさで一定にする（段階の中で陰影を付けない）
    const float3 bright = albedo / PI * radiance;
    const float3 dark   = shadeFloor ? bright * shadeColor : float3(0.0f, 0.0f, 0.0f);
    float3       color  = lerp(dark, bright, lit);

    //----------------------------------------------------------
    // ハイライト。法線とハーフベクトルが揃った狭い所だけをくっきり白くする
    //----------------------------------------------------------
    if(specularSize > 0.0f) {
        const float3 H         = normalize(V + L);
        const float  NdotH     = saturate(dot(N, H));
        const float  highlight = ToonStep(NdotH, 1.0f - specularSize * 0.1f, softness * 0.1f) * lit;
        color += radiance / PI * highlight;
    }
    return color;
}

#endif    // TSUKINO_TOON_HLSLI

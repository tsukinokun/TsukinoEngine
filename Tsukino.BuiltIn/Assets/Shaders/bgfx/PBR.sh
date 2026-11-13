//--------------------------------------------------------------
//! @file   PBR.sh
//! @brief  PBR のライティングと法線の扱い（PBR.hlsli の bgfx 版）
//! @note   影（カスケードシャドウマップ）を読む関数は、影のパスを移すとき（段階 1-4 c）に足す
//--------------------------------------------------------------
#ifndef TSUKINO_PBR_SH
#define TSUKINO_PBR_SH

#include "Scene.sh"

#define PI 3.14159265358979323846

// 影の中でも残す明るさ（PBR の直接光。HLSL 版と同じ）
#define kShadowMinLit 0.25

//--------------------------------------------------------------
//! @brief  GGX の法線分布
//--------------------------------------------------------------
float D_GGX(float NdotH, float a)
{
    float a2    = a * a;
    float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

//--------------------------------------------------------------
//! @brief  Smith の幾何減衰（Schlick-GGX）
//--------------------------------------------------------------
float G_Smith(float NdotV, float NdotL, float k)
{
    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);
    return ggx1 * ggx2;
}

//--------------------------------------------------------------
//! @brief  Schlick のフレネル
//--------------------------------------------------------------
vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (vec3_splat(1.0) - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

//--------------------------------------------------------------
//! @brief  1つの光の直接光を PBR で求める（PBR.hlsli と同じ式）
//--------------------------------------------------------------
vec3 EvaluatePBR(vec3 N, vec3 V, vec3 L, vec3 albedo, float metal, float rough0, float spec, vec3 radiance)
{
    vec3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V)) + 1e-5;
    float NdotH = saturate(dot(N, H));
    float HdotV = saturate(dot(H, V));

    float rough = max(rough0, 0.15);    // 0に近いと分母が発散するので下限を設ける
    float a     = rough * rough;
    float k     = (rough + 1.0) * (rough + 1.0) / 8.0;

    vec3 F0 = mix(vec3_splat(0.08 * spec), albedo, vec3_splat(metal));

    float D = D_GGX(NdotH, a);
    float G = G_Smith(NdotV, NdotL, k);
    vec3  F = F_Schlick(HdotV, F0);

    vec3 specTerm = (D * G * F) / (4.0 * NdotV * NdotL + 1e-5);
    vec3 kD       = (vec3_splat(1.0) - F) * (1.0 - metal);
    vec3 diff     = kD * albedo / PI;

    return (diff + specTerm) * radiance * NdotL;
}

//--------------------------------------------------------------
//! @brief  法線を 0〜1 へ詰める / 戻す
//--------------------------------------------------------------
vec3 EncodeNormal(vec3 n)
{
    return n * 0.5 + 0.5;
}

vec3 DecodeNormal(vec3 enc)
{
    return normalize(enc * 2.0 - 1.0);
}

//--------------------------------------------------------------
//! @brief  法線マップを、接線を持たないメッシュに画面の微分で当てる（PBR.hlsli と同じ）
//--------------------------------------------------------------
vec3 ApplyNormalMap(vec3 N, vec3 worldPos, vec2 uv, vec3 tangentNormal)
{
    // HLSL 版の ddx / ddy と同じ向きにする（bgfx の dFdy は HLSL では -ddy になる）
    vec3 dp1  = dFdx(worldPos);
    vec3 dp2  = -dFdy(worldPos);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = -dFdy(uv);

    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T       = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B       = dp2perp * duv1.y + dp1perp * duv2.y;

    float maxLenSq = max(dot(T, T), dot(B, B));
    float invmax   = (maxLenSq > 0.0) ? inversesqrt(maxLenSq) : 0.0;

    // HLSL の mul(tangentNormal, float3x3(T, B, N)) = 行 T・B・N の重ね合わせ
    vec3 n = tangentNormal.x * (T * invmax) + tangentNormal.y * (B * invmax) + tangentNormal.z * N;

    float lenSq = dot(n, n);
    return (lenSq > 1e-12) ? n * inversesqrt(lenSq) : N;
}

#endif    // TSUKINO_PBR_SH

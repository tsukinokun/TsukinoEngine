//--------------------------------------------------------------
//! @file   IBLBake.sh
//! @brief  環境光（IBL）のベイクで共用する処理
//! @note   HLSL 版は BRDFIntegrate.ps.hlsl と SpecularPrefilter.ps.hlsl に同じ関数を1つずつ持っていた。
//!         bgfx 版はここへまとめる
//--------------------------------------------------------------
#ifndef TSUKINO_IBL_BAKE_SH
#define TSUKINO_IBL_BAKE_SH

#define IBL_PI 3.14159265358979

//--------------------------------------------------------------
//! @brief  Van der Corput 基数逆順列（低差異列 Hammersley の生成に使う）
//--------------------------------------------------------------
float RadicalInverse_VdC(uint bitsIn)
{
    uint bits = bitsIn;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;    // / 0x100000000
}

//--------------------------------------------------------------
//! @brief  i 番目の Hammersley 点（[0,1)^2 の低差異列）
//--------------------------------------------------------------
vec2 Hammersley(uint i, uint n)
{
    return vec2(float(i) / float(n), RadicalInverse_VdC(i));
}

//--------------------------------------------------------------
//! @brief  GGX 分布に従うハーフベクトルを重要度サンプリングする
//--------------------------------------------------------------
vec3 ImportanceSampleGGX(vec2 xi, vec3 n, float rough)
{
    float a = rough * rough;

    float phi      = 2.0 * IBL_PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 h = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    // 接空間 → ワールド空間
    vec3 up        = (abs(n.z) < 0.999) ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, n));
    vec3 bitangent = cross(n, tangent);

    return normalize(tangent * h.x + bitangent * h.y + n * h.z);
}

#endif    // TSUKINO_IBL_BAKE_SH

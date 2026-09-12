//--------------------------------------------------------------
//! @file   SpecularPrefilter.ps.hlsl
//! @brief  IBLベイク：キャプチャキューブからプレフィルタ済みスペキュラキューブへの畳み込み
//! @author 山﨑愛
//! @note   VSはTonemap.vs.hlslを共用するフルスクリーン三角形。
//!         Renderer::ExecuteIBLSpecularPrefilterPass()が面×mipの回数だけ描画する。
//!         出力先の面はCBufferScene(b0)のinvViewProjが、ラフネス/サンプル数は
//!         CBufferIBLBake(b11)が決める（どちらもmip・面ごとにC++側で更新してから呼ぶ）。
//!         split-sum近似（Karis, "Real Shading in Unreal Engine 4"）のプレフィルタ項。
//--------------------------------------------------------------
#pragma pack_matrix(row_major)
#include "PBR.hlsli"

//--------------------------------------------------------------
//! @brief キャプチャキューブ（大気散乱スカイを6面へ焼いたもの）
//--------------------------------------------------------------
TextureCube  iblCaptureSource : register(t20);
SamplerState iblBakeSampler : register(s10);

//--------------------------------------------------------------
//! @brief このmipのベイクパラメータ (b11)
//--------------------------------------------------------------
cbuffer CBufferIBLBake : register(b11)
{
    float        bakeRoughness;
    uint         bakeSampleCount;
    float2       bakePad;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief Van der Corput基数逆順列（低差異列 Hammersley の生成に使う）
//--------------------------------------------------------------
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;    // / 0x100000000
}

//--------------------------------------------------------------
//! @brief i番目のHammersley点（[0,1)^2の低差異列）
//--------------------------------------------------------------
float2 Hammersley(uint i, uint n)
{
    return float2(float(i) / float(n), RadicalInverse_VdC(i));
}

//--------------------------------------------------------------
//! @brief GGX分布に従うハーフベクトルを重要度サンプリングする
//--------------------------------------------------------------
float3 ImportanceSampleGGX(float2 xi, float3 n, float roughness)
{
    float a = roughness * roughness;

    float phi      = 2.0f * PI * xi.x;
    float cosTheta = sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 h;
    h.x = cos(phi) * sinTheta;
    h.y = sin(phi) * sinTheta;
    h.z = cosTheta;

    // 接空間 → ワールド空間
    float3 up      = (abs(n.z) < 0.999f) ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent   = normalize(cross(up, n));
    float3 bitangent = cross(n, tangent);

    return normalize(tangent * h.x + bitangent * h.y + n * h.z);
}

float4 PSMain(PSInput input) : SV_TARGET
{
    //----------------------------------------------------------
    // このテクセルが担当する方向を復元する（IrradianceConvolve.ps.hlslと同じ手法）。
    // split-sum近似の慣例に従い N = V = R とする
    //----------------------------------------------------------
    float2 ndc      = input.uv * 2.0f - 1.0f;
    float4 worldPos = mul(float4(ndc, 0.0f, 1.0f), invViewProj);
    worldPos /= worldPos.w;
    float3 N = normalize(worldPos.xyz - cameraPos.xyz);
    float3 V = N;

    float3 prefilteredColor = float3(0.0f, 0.0f, 0.0f);
    float  totalWeight      = 0.0f;

    [loop]
    for(uint i = 0; i < bakeSampleCount; ++i) {
        float2 xi = Hammersley(i, bakeSampleCount);
        float3 H  = ImportanceSampleGGX(xi, N, bakeRoughness);
        float3 L  = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = saturate(dot(N, L));
        if(NdotL > 0.0f) {
            prefilteredColor += iblCaptureSource.SampleLevel(iblBakeSampler, L, 0.0f).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor /= max(totalWeight, 1e-4f);

    return float4(prefilteredColor, 1.0f);
}

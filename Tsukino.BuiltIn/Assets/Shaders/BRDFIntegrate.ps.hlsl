//--------------------------------------------------------------
//! @file   BRDFIntegrate.ps.hlsl
//! @brief  IBLベイク：split-sum近似のBRDF積分LUT生成
//! @author 山﨑愛
//! @note   VSはTonemap.vs.hlslを共用するフルスクリーン三角形。
//!         スカイに一切依存しないため、Renderer::Initialize時に一度だけ描画すればよい
//!         （キャプチャ/irradiance/プレフィルタの一発ベイクとは独立）。
//!         入力uv.x=NdotV、uv.y=roughnessとして、出力rg=(スケールA, バイアスB)を書く
//!         （Karis, "Real Shading in Unreal Engine 4"のsplit-sum近似）。
//--------------------------------------------------------------
#pragma pack_matrix(row_major)

static const float PI = 3.14159265358979f;

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief Van der Corput基数逆順列（SpecularPrefilter.ps.hlslと同じ実装）
//--------------------------------------------------------------
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;
}

float2 Hammersley(uint i, uint n)
{
    return float2(float(i) / float(n), RadicalInverse_VdC(i));
}

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

    float3 up        = (abs(n.z) < 0.999f) ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent   = normalize(cross(up, n));
    float3 bitangent = cross(n, tangent);

    return normalize(tangent * h.x + bitangent * h.y + n * h.z);
}

//--------------------------------------------------------------
//! @brief IBL用Smith幾何減衰（k = roughness^2 / 2。直接照明用のG_Smithとはkの式が異なる）
//--------------------------------------------------------------
float GeometrySchlickGGX_IBL(float ndotX, float roughness)
{
    float a = roughness * roughness;
    float k = (a * a) / 2.0f;
    return ndotX / (ndotX * (1.0f - k) + k);
}

float GeometrySmith_IBL(float ndotV, float ndotL, float roughness)
{
    return GeometrySchlickGGX_IBL(ndotV, roughness) * GeometrySchlickGGX_IBL(ndotL, roughness);
}

//--------------------------------------------------------------
//! @brief (NdotV, roughness) 1点ぶんのsplit-sum積分
//--------------------------------------------------------------
float2 IntegrateBRDF(float ndotV, float roughness)
{
    float3 V;
    V.x = sqrt(1.0f - ndotV * ndotV);
    V.y = 0.0f;
    V.z = ndotV;

    float A = 0.0f;
    float B = 0.0f;

    float3 N = float3(0.0f, 0.0f, 1.0f);

    const uint SAMPLE_COUNT = 1024u;

    [loop]
    for(uint i = 0; i < SAMPLE_COUNT; ++i) {
        float2 xi = Hammersley(i, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(xi, N, roughness);
        float3 L  = normalize(2.0f * dot(V, H) * H - V);

        float ndotL = max(L.z, 0.0f);
        float ndotH = max(H.z, 0.0f);
        float vdotH = max(dot(V, H), 0.0f);

        if(ndotL > 0.0f) {
            float g     = GeometrySmith_IBL(ndotV, ndotL, roughness);
            float gVis  = (g * vdotH) / max(ndotH * ndotV, 1e-4f);
            float fc    = pow(1.0f - vdotH, 5.0f);

            A += (1.0f - fc) * gVis;
            B += fc * gVis;
        }
    }

    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return float2(A, B);
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float ndotV     = saturate(input.uv.x) + 1e-4f;
    float roughness = saturate(input.uv.y);

    float2 result = IntegrateBRDF(ndotV, roughness);
    return float4(result, 0.0f, 1.0f);
}

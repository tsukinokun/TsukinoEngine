//--------------------------------------------------------------
//! @file   Model.ps.hlsl
//! @brief  3Dモデル用のピクセルシェーダ
//! @author 山﨑愛
//--------------------------------------------------------------

cbuffer CBufferScene : register(b0)
{
    matrix view;
    matrix projection;
    matrix viewProj;
};

cbuffer CBufferTransform : register(b1)
{
    matrix world;
};

cbuffer CBufferMaterial : register(b2)
{
    float4 baseColor;
    float3 emissive;
    float metallic;
    float roughness;
    float3 padding;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    // Basic direction light (hardcoded for now)
    float3 lightDir = normalize(float3(1.0f, -1.0f, 1.0f));
    float ndotl = saturate(dot(input.normal, lightDir));
    
    // Very basic diffuse lighting
    float3 ambient = float3(0.2f, 0.2f, 0.2f);
    float3 diffuse = ndotl.xxx;
    
    float3 finalColor = baseColor.rgb * (ambient + diffuse) + emissive;
    
    return float4(finalColor, baseColor.a);
}

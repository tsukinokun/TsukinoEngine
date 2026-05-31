//--------------------------------------------------------------
//! @file   Model.ps.hlsl
//! @brief  3Dモデル用のピクセルシェーダ
//! @author 山﨑愛
//--------------------------------------------------------------

//--------------------------------------------------------------
//! @brief  シーン用定数バッファ
//--------------------------------------------------------------
cbuffer CBufferScene : register(b0)
{
    matrix view;
    matrix projection;
    matrix viewProj;
};

//--------------------------------------------------------------
//! @brief  モデル位置定数バッファ
//--------------------------------------------------------------
cbuffer CBufferTransform : register(b1)
{
    matrix world;
};

//--------------------------------------------------------------
//! @brief  マテリアル定数バッファ
//--------------------------------------------------------------
cbuffer CBufferMaterial : register(b2)
{
    float4 baseColor;
    float3 emissive;
    float metallic;
    float roughness;
    float3 padding;
};

//--------------------------------------------------------------
//! @brief  アルベドテクスチャ
//--------------------------------------------------------------
Texture2D albedoTexture : register(t0);

//--------------------------------------------------------------
//! @brief  アルベドテクスチャ用サンプラー
//--------------------------------------------------------------
SamplerState albedoSampler : register(s0);

//--------------------------------------------------------------
//! @brief  ピクセルシェーダ入力構造体
//--------------------------------------------------------------
struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief  ピクセルシェーダメイン関数
//--------------------------------------------------------------
float4 PSMain(PSInput input) : SV_TARGET
{
     // albedoテクスチャをサンプリングしてbaseColorと乗算
    float4 albedo = albedoTexture.Sample(albedoSampler, input.uv) * baseColor;

    // Basic direction light (hardcoded for now)
    float3 lightDir = normalize(float3(1.0f, -1.0f, 1.0f));
    float ndotl = saturate(dot(input.normal, -lightDir));

    // Very basic diffuse lighting
    float3 ambient = float3(0.2f, 0.2f, 0.2f);
    float3 diffuse = ndotl.xxx;

    float3 finalColor = albedo.rgb * (ambient + diffuse) + emissive;

    
    return float4(finalColor, baseColor.a);
}

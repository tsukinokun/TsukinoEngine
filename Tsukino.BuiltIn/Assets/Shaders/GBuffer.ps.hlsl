//--------------------------------------------------------------
//! @file   GBuffer.ps.hlsl
//! @brief  ディファードGBufferパス用ピクセルシェーダ
//! @author 山﨑愛
//! @note   VSはModel.vs.hlsl（スキン有り）/ ModelStatic.vs.hlsl（スキン無し）を
//!         共用する。ライティングはここでは計算せず、後段のLightingパスへ委譲する。
//--------------------------------------------------------------
#pragma pack_matrix(row_major)
#include "PBR.hlsli"

//--------------------------------------------------------------
//! @brief マテリアル定数バッファ (b2)
//--------------------------------------------------------------
cbuffer CBufferMaterial : register(b2)
{
    float4 baseColor;
    float3 emissive;
    float  metallic;
    float  roughness;
    float  specular;
    float4 rimColor;     // xyz: ふちの色, w: ふちの強さ
    float4 rimParams;    // x: ふちの鋭さ(pow指数), y: 全体の白発光量, zw: 予約
};

//--------------------------------------------------------------
//! @brief アルベドテクスチャ (t0)
//--------------------------------------------------------------
Texture2D     albedoTexture : register(t0);
SamplerState  albedoSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief G-Buffer出力
//--------------------------------------------------------------
struct PSOutput
{
    float4 albedo      : SV_TARGET0;    // rgb: albedo, a: 未使用
    float4 normal      : SV_TARGET1;    // rgb: ワールド法線(エンコード済み), a: 未使用（将来のShadingModel ID等に予約）
    float4 material    : SV_TARGET2;    // r: metallic, g: roughness, b: specular, a: AO（Phase 3でaoHandle対応まで1.0固定）
    float4 emissiveOut : SV_TARGET3;    // rgb: emissive + リム発光 + 全体白発光, a: 未使用
    float4 worldPosOut : SV_TARGET4;    // rgb: ワールド座標（頂点シェーダー補間値そのまま）, a: 未使用
};

PSOutput PSMain(PSInput input)
{
    PSOutput output;

    //----------------------------------------------------------
    // アルベド
    // Model.ps.hlsl（フォワード）と同じくACESを通す。トーンマップとの
    // 二重掛けだが、既存の見た目との差分をゼロにするため意図的に踏襲する
    // （除去は別途ポストプロセス整備のタイミングで扱う）
    //----------------------------------------------------------
    float4 albedoSample = albedoTexture.Sample(albedoSampler, input.uv);
    float3 albedo        = ACES(albedoSample.rgb * baseColor.rgb);

    float3 N = normalize(input.normal);

    //----------------------------------------------------------
    // リム発光・全体白発光（Model.ps.hlslのハイライト演出と同じ式）
    // Lightingパス側の計算を単純化するため、この段階で焼き込んでおく
    //----------------------------------------------------------
    float3 V     = normalize(cameraPos.xyz - input.worldPos);
    float  NdotV = saturate(dot(N, V)) + 1e-5f;
    float  rim   = pow(saturate(1.0f - NdotV), rimParams.x);

    float3 emissiveTotal = emissive + rimColor.rgb * rim * rimColor.w + rimParams.y;

    output.albedo      = float4(albedo, albedoSample.a * baseColor.a);
    output.normal      = float4(EncodeNormal(N), 0.0f);
    output.material    = float4(metallic, roughness, specular, 1.0f);
    output.emissiveOut = float4(emissiveTotal, 0.0f);
    output.worldPosOut = float4(input.worldPos, 0.0f);

    return output;
}

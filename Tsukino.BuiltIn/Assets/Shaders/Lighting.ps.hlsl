//--------------------------------------------------------------
//! @file   Lighting.ps.hlsl
//! @brief  ディファードLightingパス用ピクセルシェーダ
//! @author 山﨑愛
//! @note   VSはTonemap.vs.hlslを共用する（頂点バッファ不要のフルスクリーン三角形）。
//!         G-Bufferと深度を読み、HDRバッファへ直接1回で全ライトを加算する。
//--------------------------------------------------------------
#pragma pack_matrix(row_major)
#include "Lighting.hlsli"

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 PSMain(PSInput input) : SV_TARGET
{
    //----------------------------------------------------------
    // フルスクリーン三角形のUVはTonemapパスと同じ生成規則。
    // テクスチャ座標系（Y下向き）に合わせて反転する
    //----------------------------------------------------------
    float2 screenUV = float2(input.uv.x, 1.0f - input.uv.y);

    float depth = gbufferDepth.Sample(gbufferSampler, screenUV).r;

    //----------------------------------------------------------
    // リバースZ：深度0 = 何も描かれていない背景ピクセル（Skyパスの結果を保持する）
    //----------------------------------------------------------
    if(depth <= 0.0f)
        discard;

    //----------------------------------------------------------
    // ワールド座標はGBufferパスの頂点シェーダー補間値をそのまま読む
    // （深度からの再構成はリバースZ+遠距離で精度が崩れるため使わない）
    //----------------------------------------------------------
    float3 worldPos = gbufferWorldPos.Sample(gbufferSampler, screenUV).xyz;

    float4 albedoSample   = gbufferAlbedo.Sample(gbufferSampler, screenUV);
    float3 normalSample   = gbufferNormal.Sample(gbufferSampler, screenUV).rgb;
    float4 materialSample = gbufferMaterial.Sample(gbufferSampler, screenUV);
    float3 emissiveSample = gbufferEmissive.Sample(gbufferSampler, screenUV).rgb;

    float3 albedo    = albedoSample.rgb;
    float3 N         = DecodeNormal(normalSample);
    float  metallic  = materialSample.r;
    float  roughness = materialSample.g;
    float  specular  = materialSample.b;
    float  ao        = materialSample.a;

    float3 V = normalize(cameraPos.xyz - worldPos);

    float3 lit = float3(0.0f, 0.0f, 0.0f);

    //----------------------------------------------------------
    // ディレクショナルライト（影付き、1灯固定）
    //----------------------------------------------------------
    {
        float3 L      = normalize(-lightDir.xyz);    // lightDirは「ライトが向いている方向」なので反転
        float  shadow = GetShadowPCF(worldPos);

        // 影の値を「0.0〜1.0」ではなく「minShadow〜1.0」の範囲にする
        float minShadow = 0.25f;
        shadow           = max(shadow, minShadow);

        float3 radiance = lightColor.rgb * lightColor.w * shadow;
        lit += EvaluatePBR(N, V, L, albedo, metallic, roughness, specular, radiance);
    }

    //----------------------------------------------------------
    // 点光源・スポットライト（影なし、最大 TSUKINO_MAX_LIGHTS 灯）
    //----------------------------------------------------------
    for(uint i = 0; i < lightCount; ++i) {
        GPULight light = lights[i];

        float3 toLight = light.positionRange.xyz - worldPos;
        float  dist    = length(toLight);
        float3 L       = toLight / max(dist, 1e-5f);

        float atten = Attenuate(dist, light.positionRange.w);

        // スポットライトは円錐の外側を減衰させる
        if(light.directionType.w > 0.5f) {
            float3 spotDir   = normalize(light.directionType.xyz);
            float  cosAngle  = dot(-L, spotDir);
            float  spotAtten = saturate((cosAngle - light.spotParams.y) / max(light.spotParams.x - light.spotParams.y, 1e-4f));
            atten *= spotAtten * spotAtten;
        }

        float3 radiance = light.colorIntensity.rgb * light.colorIntensity.a * atten;
        lit += EvaluatePBR(N, V, L, albedo, metallic, roughness, specular, radiance);
    }

    //----------------------------------------------------------
    // アンビエント（IBLの代わりの定数環境光。AOで遮蔽）
    //----------------------------------------------------------
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * ao;

    float3 finalColor = ambient + lit + emissiveSample;

    return float4(finalColor, albedoSample.a);
}

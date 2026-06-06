//--------------------------------------------------------------
//! @file   Model.ps.hlsl
//! @brief  3Dモデル用のピクセルシェーダ
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma pack_matrix(row_major)
//--------------------------------------------------------------
//! @brief シーン用定数バッファ
//--------------------------------------------------------------
cbuffer CBufferScene : register(b0)
{
    matrix view;
    matrix projection;
    matrix viewProj;
    matrix lightViewProj; // ライト空間のViewProjection行列
    float4 lightDir; // xyz: ライト方向（正規化済み）
};
//--------------------------------------------------------------
//! @brief モデル位置定数バッファ
//--------------------------------------------------------------
cbuffer CBufferTransform : register(b1)
{
    matrix world;
};
//--------------------------------------------------------------
//! @brief マテリアル定数バッファ
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
//! @brief アルベドテクスチャ (t0)
//--------------------------------------------------------------
Texture2D albedoTexture : register(t0);
//--------------------------------------------------------------
//! @brief シャドウマップ (t1)
//--------------------------------------------------------------
Texture2D shadowMap : register(t1);
//--------------------------------------------------------------
//! @brief アルベドテクスチャ用サンプラー (s0)
//--------------------------------------------------------------
SamplerState albedoSampler : register(s0);
//--------------------------------------------------------------
//! @brief シャドウマップ用比較サンプラー (s1)
//--------------------------------------------------------------
SamplerComparisonState shadowSampler : register(s1);
//--------------------------------------------------------------
//! @brief ピクセルシェーダ入力構造体
//--------------------------------------------------------------
struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};
//--------------------------------------------------------------
//! @brief PCFシャドウサンプリング
//! @return 遮蔽量 0.0f(暗) - 1.0f(明)
//--------------------------------------------------------------
float GetShadowPCF(float3 worldPos)
{
    //----------------------------------------------------------
    // ワールド座標をライト空間に変換
    //----------------------------------------------------------
    float4 lightSpace = mul(float4(worldPos, 1.0f), lightViewProj);

    //----------------------------------------------------------
    // クリップ座標をUV座標に変換
    // x: [-1, 1] → [0, 1]
    // y: [-1, 1] → [1, 0] （DirectXはY反転）
    //----------------------------------------------------------
    float2 uv = lightSpace.xy * float2(0.5f, -0.5f) + 0.5f;

    // UV範囲外は影なし
    if (any(uv < 0.0f) || any(1.0f < uv))
        return 1.0f;

    // 深度値 + シャドウアクネ対策バイアス
    float depth = lightSpace.z - 0.001f;

    //----------------------------------------------------------
    // 3x3 PCF : 9サンプルの平均を取る
    //----------------------------------------------------------
    float shadow = 0.0f;
    float texelSize = 1.0f / 2048.0f; // SHADOW_MAP_SIZEに合わせる

    [unroll]
    for (int x = -1; x <= 1; x++)
    {
        [unroll]
        for (int y = -1; y <= 1; y++)
        {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.SampleCmpLevelZero(
                shadowSampler, uv + offset, depth);
        }
    }

    return shadow / 9.0f;
}
//--------------------------------------------------------------
//! @brief ピクセルシェーダメイン関数
//--------------------------------------------------------------
float4 PSMain(PSInput input) : SV_TARGET
{
    // アルベドテクスチャをサンプリングしてbaseColorと乗算
    float4 albedo = albedoTexture.Sample(albedoSampler, input.uv) * baseColor;

    // ライティング
    float3 ld = normalize(lightDir.xyz);
    float NdotL = saturate(dot(input.normal, -ld));

    // シャドウ係数を取得（0.0:影 ～ 1.0:明）
    float shadow = GetShadowPCF(input.worldPos);

    // アンビエント + ディフューズ（シャドウ適用）
    float3 ambient = float3(0.3f, 0.3f, 0.3f);
    float3 diffuse = NdotL * 1.5f * shadow;
    float3 finalColor = albedo.rgb * (ambient + diffuse) + emissive;

    return float4(finalColor, baseColor.a);
}

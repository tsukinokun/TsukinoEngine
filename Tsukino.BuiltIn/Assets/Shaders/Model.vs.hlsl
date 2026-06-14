//--------------------------------------------------------------
//! @file   Model.vs.hlsl
//! @brief  3Dモデル用の頂点シェーダ
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
    matrix invViewProj;
    matrix lightViewProj; // ライト空間のViewProjection行列
    float4 lightDir; // xyz: ライト方向（正規化済み）
    float4 lightColor; // xyz: ライトの色, w: 未使用
    float4 cameraPos; // xyz: カメラのワールド座標, w: 未使用
};


//--------------------------------------------------------------
//! @brief トランスフォーム定数バッファ
//--------------------------------------------------------------
cbuffer CBufferTransform : register(b1)
{
    matrix world;
};

//--------------------------------------------------------------
//! @brief ボーン行列定数バッファ (b3)
//--------------------------------------------------------------
cbuffer CBufferSkinning : register(b3)
{
    matrix bones[128]; // 最大128ボーン対応
};

//--------------------------------------------------------------
//! @brief VS入力構造体
//--------------------------------------------------------------
struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    uint4 indices : BONE_INDICES; // ボーンインデックス (0~127)
    float4 weights : BONE_WEIGHTS; // ボーンウェイト (合計が 1.0 になる)
};

//--------------------------------------------------------------
//! @brief VS出力構造体
//--------------------------------------------------------------
struct VSOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    // ------------------------------------------------------------
    // 4ボーンの影響を線形補間（Linear Blend Skinning）して合成行列を作成
    // ------------------------------------------------------------
    matrix skinMatrix =
        bones[input.indices.x] * input.weights.x +
        bones[input.indices.y] * input.weights.y +
        bones[input.indices.z] * input.weights.z +
        bones[input.indices.w] * input.weights.w;

    // 1. スキニング行列でローカル座標からボーン変形後座標へ
    // 2. その後、通常のワールド行列を適用
    float4 localPos = float4(input.position, 1.0f);
    float4 skinnedPos = mul(localPos, skinMatrix);
    float4 worldPos = mul(skinnedPos, world);
    
    output.worldPos = worldPos.xyz;
    output.position = mul(worldPos, viewProj);
    
    // 法線（Normal）も同様にボーン変形をしてからワールド座標系へ変換
    // ※スケールが入ることを考慮して後で再正規化
    float3 skinnedNormal = mul(input.normal, (float3x3) skinMatrix);
    output.normal = normalize(mul(skinnedNormal, (float3x3) world));
    
    output.uv = input.uv;
    
    return output;
}

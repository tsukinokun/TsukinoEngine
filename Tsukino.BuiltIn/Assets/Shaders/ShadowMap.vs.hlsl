//--------------------------------------------------------------
//! @file   ShadowMap.vs.hlsl
//! @brief  シャドウマップ生成用頂点シェーダ（スケルタルメッシュ）
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
//! @brief ボーン行列定数バッファ
//--------------------------------------------------------------
cbuffer CBufferSkinning : register(b3)
{
    matrix bones[128];
};
//--------------------------------------------------------------
//! @brief VS入力構造体
//--------------------------------------------------------------
struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    uint4 boneIndices : BONE_INDICES;
    float4 boneWeights : BONE_WEIGHTS;
};

float4 VSMain(VSInput input) : SV_POSITION
{
    // スキニング行列の合成
    matrix skinMatrix =
        bones[input.boneIndices.x] * input.boneWeights.x +
        bones[input.boneIndices.y] * input.boneWeights.y +
        bones[input.boneIndices.z] * input.boneWeights.z +
        bones[input.boneIndices.w] * input.boneWeights.w;

    float4 worldPos = mul(mul(float4(input.position, 1.0f), skinMatrix), world);
    return mul(worldPos, lightViewProj);
}

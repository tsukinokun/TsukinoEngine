//--------------------------------------------------------------
//! @file   ShadowMap.vs.hlsl
//! @brief  シャドウマップ生成用頂点シェーダ（スケルタルメッシュ）
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma pack_matrix(row_major)
// シーン定数バッファ(b0)。手書きせずScene.hlsliから取り込む（理由は同ファイル参照）
#include "Scene.hlsli"

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

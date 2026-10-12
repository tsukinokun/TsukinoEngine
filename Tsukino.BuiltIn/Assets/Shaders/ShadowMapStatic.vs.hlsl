//--------------------------------------------------------------
//! @file   ShadowMapStatic.vs.hlsl
//! @brief  シャドウマップ生成用頂点シェーダ（スタティックメッシュ）
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
//! @brief VS入力構造体
//--------------------------------------------------------------
struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief VS出力構造体（くり抜きの影のため、ピクセルシェーダーへ UV も渡す）
//--------------------------------------------------------------
struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOutput VSMain(VSInput input)
{
    float4 worldPos = mul(float4(input.position, 1.0f), world);
    // シャドウパスが描画中のカスケード番号はshadowParams.zに入っている。
    // こうしておくとこのシェーダーはカスケードの枚数を知らなくて済む
    VSOutput output;
    output.position = mul(worldPos, cascadeViewProj[(uint)shadowParams.z]);
    output.uv       = input.uv;
    return output;
}

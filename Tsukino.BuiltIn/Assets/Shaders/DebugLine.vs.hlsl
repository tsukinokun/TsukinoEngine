//--------------------------------------------------------------
//! @file   DebugLine.vs.hlsl
//! @brief  デバッグライン用の頂点シェーダ
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma pack_matrix(row_major)

//--------------------------------------------------------------
//! @brief シーン定数バッファ
//--------------------------------------------------------------
cbuffer CBufferScene : register(b0)
{
    matrix view;
    matrix projection;
    matrix viewProj;
};

//--------------------------------------------------------------
//! @brief 頂点シェーダー入力構造体
//--------------------------------------------------------------
struct VSInput
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};

//--------------------------------------------------------------
//! @brief 頂点シェーダー出力構造体
//--------------------------------------------------------------
struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

//--------------------------------------------------------------
//! @brief  デバッグライン用頂点シェーダー
//! @param  input [in] 頂点シェーダーの入力
//! @return 出力構造体
//--------------------------------------------------------------
VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 1.0f), viewProj);
    output.Color = input.Color;
    return output;
}

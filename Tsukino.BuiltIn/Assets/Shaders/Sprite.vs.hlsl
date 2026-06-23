//--------------------------------------------------------------
//! @file   Sprite.vs.hlsl
//! @brief  スプライト用頂点シェーダ
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma pack_matrix(row_major)
//--------------------------------------------------------------
// 定数バッファ：シーン
//--------------------------------------------------------------
cbuffer CBufferScene : register(b0)
{
    matrix View;
    matrix Projection;
    matrix ViewProj;
};

//--------------------------------------------------------------
// 定数バッファ：位置
//--------------------------------------------------------------
cbuffer TransformBuffer : register(b1)
{
    float4x4 World;
};

//--------------------------------------------------------------
// 入力構造体
//--------------------------------------------------------------
struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
// 出力構造体
//--------------------------------------------------------------
struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief  スプライト用頂点シェーダー
//! @param  input [in] 頂点シェーダーの入力
//! @return 出力構造体
//--------------------------------------------------------------
VSOutput VSMain(VSInput input)
{
    VSOutput output; // 出力構造体の宣言
    // 頂点 * ワールド行列 * ビュープロジェクション行列
    output.position = mul(float4(input.position, 1.0f), mul(World, ViewProj));
    
    // Y座標を反転（DirectXの座標系に合わせるため）
    output.position.y = -output.position.y;
    
    output.uv = float2(input.uv.x, 1.0f - input.uv.y);
     
    return output;
}

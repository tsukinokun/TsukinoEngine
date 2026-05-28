//--------------------------------------------------------------
//! @file   ModelStatic.vs.hlsl
//! @brief  3Dモデル用のアニメーションなし頂点用シェーダ
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

    // ワールド座標へ変換！
    float4 worldPos = mul(float4(input.position, 1.0f), world);
    
    output.worldPos = worldPos.xyz;
    output.position = mul(worldPos, viewProj);
    output.normal = normalize(mul(input.normal, (float3x3) world));
    output.uv = input.uv;
    
    return output;
}

//--------------------------------------------------------------
//! @file   ModelStatic.vs.hlsl
//! @brief  3Dモデル用のアニメーションなし頂点用シェーダ
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
    matrix prevViewProj; // 前フレームのViewProjection行列（速度バッファ生成用）
};


//--------------------------------------------------------------
//! @brief トランスフォーム定数バッファ
//--------------------------------------------------------------
cbuffer CBufferTransform : register(b1)
{
    matrix world;
    matrix prevWorld; // 前フレームのワールド行列
    float4 motionFlags; // x: 1=前フレーム有効 / 0=速度ゼロ
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
//! @note  curClip / prevClip はモーションブラーの速度計算専用。
//!        Model.vs.hlsl と同じシグネチャに揃えてある。
//--------------------------------------------------------------
struct VSOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 curClip : TEXCOORD1;
    float4 prevClip : TEXCOORD2;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4 localPos = float4(input.position, 1.0f);

    // ワールド座標へ変換！
    float4 worldPos = mul(localPos, world);
    
    output.worldPos = worldPos.xyz;
    output.curClip = mul(worldPos, viewProj);
    output.position = output.curClip;
    output.normal = normalize(mul(input.normal, (float3x3) world));
    output.uv = input.uv;

    // ------------------------------------------------------------
    // 前フレームのクリップ座標（速度バッファ用）
    // ------------------------------------------------------------
    if (motionFlags.x > 0.5f)
    {
        float4 prevWorldPos = mul(localPos, prevWorld);
        output.prevClip = mul(prevWorldPos, prevViewProj);
    }
    else
    {
        output.prevClip = output.curClip;
    }
    
    return output;
}

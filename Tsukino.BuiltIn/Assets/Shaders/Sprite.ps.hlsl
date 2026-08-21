//--------------------------------------------------------------
//! @file   Sprite.ps.hlsl
//! @brief  スプライト用ピクセルシェーダ
//! @author 山﨑愛
//--------------------------------------------------------------

Texture2D u_Texture : register(t0); // テクスチャ
SamplerState u_Sampler : register(s0); // サンプラー

//--------------------------------------------------------------
// 定数バッファ：マテリアル（b2）。SpriteComponent::tintColorがbaseColorとして渡ってくる。
// レイアウトはCBufferMaterial（Tsukino/Renderer/ConstantBuffer.hpp）と一致させること。
// スプライトではbaseColor以外は使わない
//--------------------------------------------------------------
cbuffer CBufferMaterial : register(b2)
{
    float4 baseColor;
    float3 emissive;
    float metallic;
    float roughness;
    float specular;
    float4 rimColor;
    float4 rimParams;
};

//--------------------------------------------------------------
// 頂点シェーダーから受け取った情報の構造体
//--------------------------------------------------------------
struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief  スプライト用ピクセルシェーダー
//! @param  input [in] 頂点シェーダーからの出力
//! @return ピクセルカラー
//--------------------------------------------------------------
float4 PSMain(PSInput input) : SV_TARGET
{
    return u_Texture.Sample(u_Sampler, input.uv) * baseColor;
}

//--------------------------------------------------------------
//! @file   Sprite.ps.hlsl
//! @brief  スプライト用ピクセルシェーダ
//! @author 山﨑愛
//--------------------------------------------------------------

Texture2D u_Texture : register(t0); // テクスチャ
SamplerState u_Sampler : register(s0); // サンプラー

//--------------------------------------------------------------
// 定数バッファ：マテリアル（b2）。SpriteComponent::tintColorがbaseColorとして、
// SpriteComponent::fill* が spriteFill として渡ってくる。
// スプライトではこの2つ以外は使わないが、b2のレイアウトはModel系と
// 共有しているのでMaterial.hlsliから取り込む
//--------------------------------------------------------------
#include "Material.hlsli"

//--------------------------------------------------------------
// 頂点シェーダーから受け取った情報の構造体
//--------------------------------------------------------------
struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief  円形の塗り（SpriteFillMode::Radial）で、このピクセルを塗る割合を求める
//! @param  uv [in] テクスチャ座標（画面上で x が右、y が下）
//! @return 0〜1。塗らない角度は 0、境目は1ピクセル幅でなめらかに変わる。塗りなしのときは常に 1
//--------------------------------------------------------------
float RadialFillCoverage(float2 uv)
{
    const float kTwoPi = 6.28318530718f;
    if (spriteFill.x < 0.5f || spriteFill.y >= 1.0f)
        return 1.0f;

    // 中心から見た角度を、真上を 0 として時計回りに測る（atan2 の引数は画面の y が下向きなのに合わせてある）
    float2 d     = uv - 0.5f;
    float  angle = atan2(d.x, -d.y);

    // 開始角から塗る向きに何割進んだ位置か（0〜1）
    float t = frac(((angle - spriteFill.z) * spriteFill.w) / kTwoPi + 1.0f);

    // 1ピクセルあたりの割合の変化量。角度は中心に近いほど速く変わるので、半径で割って求める
    float aa = length(fwidth(d)) / (max(length(d), 1.0e-4f) * kTwoPi);
    return saturate((spriteFill.y - t) / max(aa, 1.0e-5f) + 0.5f);
}

//--------------------------------------------------------------
//! @brief  スプライト用ピクセルシェーダー
//! @param  input [in] 頂点シェーダーからの出力
//! @return ピクセルカラー
//--------------------------------------------------------------
float4 PSMain(PSInput input) : SV_TARGET
{
    float4 color = u_Texture.Sample(u_Sampler, input.uv) * baseColor;
    color.a *= RadialFillCoverage(input.uv);
    return color;
}

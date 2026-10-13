//--------------------------------------------------------------
//! @file   Bloom.hlsli
//! @brief  ブルームパスの共通定義（定数バッファ・入力・サンプリング）
//! @author 山﨑愛
//! @note   VSはTonemap.vs.hlslを共用する（頂点バッファ不要のフルスクリーン三角形）。
//!         縮小は13タップ、拡大は3x3のテントフィルタ（Call of Duty: Advanced Warfare の
//!         発表で広まった定番の組み合わせ）。縮小を1/2ずつ重ねるので、少ないタップでも
//!         画面の広い範囲まで滲みが届く
//--------------------------------------------------------------
#ifndef TSUKINO_BLOOM_HLSLI
#define TSUKINO_BLOOM_HLSLI

Texture2D    sourceTexture : register(t0);    // 読む側の絵（シーン、または1つ前の段）
SamplerState linearSampler : register(s0);    // LinearClamp

//--------------------------------------------------------------
//! @brief ブルームパラメータ (b11)
//! @note  ConstantBuffer.hpp の CBufferBloom と並びを一致させること
//--------------------------------------------------------------
cbuffer CBufferBloom : register(b11)
{
    float2 texelSize;    // 読む側の1テクセルのUVサイズ
    float  threshold;    // 明部抽出の閾値
    float  softKnee;     // 閾値付近をなだらかにする幅
    float3 tint;         // 出力に掛ける色
    float  intensity;    // 出力に掛ける強さ
    float  radius;       // 拡大時のぼかし半径の倍率
    float3 bloomPadding;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief フルスクリーン三角形のUVをテクスチャ座標（Y下向き）へ直す
//--------------------------------------------------------------
float2 ToTextureUV(float2 uv)
{
    return float2(uv.x, 1.0f - uv.y);
}

//--------------------------------------------------------------
//! @brief 13タップの縮小フィルタ
//! @note  中央の2x2ブロックを重く、外周の4ブロックを軽く平均する。
//!        単純な2x2平均よりちらつき（細い光る線が縮小段ごとに出たり消えたりする）が少ない
//--------------------------------------------------------------
float3 Downsample13(float2 uv)
{
    float2 t = texelSize;

    float3 a = sourceTexture.Sample(linearSampler, uv + t * float2(-2.0f, -2.0f)).rgb;
    float3 b = sourceTexture.Sample(linearSampler, uv + t * float2( 0.0f, -2.0f)).rgb;
    float3 c = sourceTexture.Sample(linearSampler, uv + t * float2( 2.0f, -2.0f)).rgb;
    float3 d = sourceTexture.Sample(linearSampler, uv + t * float2(-2.0f,  0.0f)).rgb;
    float3 e = sourceTexture.Sample(linearSampler, uv).rgb;
    float3 f = sourceTexture.Sample(linearSampler, uv + t * float2( 2.0f,  0.0f)).rgb;
    float3 g = sourceTexture.Sample(linearSampler, uv + t * float2(-2.0f,  2.0f)).rgb;
    float3 h = sourceTexture.Sample(linearSampler, uv + t * float2( 0.0f,  2.0f)).rgb;
    float3 i = sourceTexture.Sample(linearSampler, uv + t * float2( 2.0f,  2.0f)).rgb;
    float3 j = sourceTexture.Sample(linearSampler, uv + t * float2(-1.0f, -1.0f)).rgb;
    float3 k = sourceTexture.Sample(linearSampler, uv + t * float2( 1.0f, -1.0f)).rgb;
    float3 l = sourceTexture.Sample(linearSampler, uv + t * float2(-1.0f,  1.0f)).rgb;
    float3 m = sourceTexture.Sample(linearSampler, uv + t * float2( 1.0f,  1.0f)).rgb;

    float3 result = (j + k + l + m) * 0.125f;
    result += (a + b + d + e) * 0.03125f;
    result += (b + c + e + f) * 0.03125f;
    result += (d + e + g + h) * 0.03125f;
    result += (e + f + h + i) * 0.03125f;
    return result;
}

#endif    // TSUKINO_BLOOM_HLSLI

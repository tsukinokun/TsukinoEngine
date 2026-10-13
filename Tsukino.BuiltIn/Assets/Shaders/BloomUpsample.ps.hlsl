//--------------------------------------------------------------
//! @file   BloomUpsample.ps.hlsl
//! @brief  ブルームの拡大（小さい段 → 1つ大きい段、最後はシーンへ）
//! @author 山﨑愛
//! @note   書き込み先へは加算ブレンドで足す。途中の段では tint=1, intensity=1 で、
//!         シーンへ足す最後の1回だけ BloomComponent の色と強さが入る
//--------------------------------------------------------------
#include "Bloom.hlsli"

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 uv = ToTextureUV(input.uv);
    float2 t  = texelSize * radius;

    //----------------------------------------------------------
    // 3x3 のテントフィルタ（重み 1-2-1 / 2-4-2 / 1-2-1）
    //----------------------------------------------------------
    float3 color = sourceTexture.Sample(linearSampler, uv).rgb * 4.0f;
    color += sourceTexture.Sample(linearSampler, uv + float2(-t.x, 0.0f)).rgb * 2.0f;
    color += sourceTexture.Sample(linearSampler, uv + float2( t.x, 0.0f)).rgb * 2.0f;
    color += sourceTexture.Sample(linearSampler, uv + float2(0.0f, -t.y)).rgb * 2.0f;
    color += sourceTexture.Sample(linearSampler, uv + float2(0.0f,  t.y)).rgb * 2.0f;
    color += sourceTexture.Sample(linearSampler, uv + float2(-t.x, -t.y)).rgb;
    color += sourceTexture.Sample(linearSampler, uv + float2( t.x, -t.y)).rgb;
    color += sourceTexture.Sample(linearSampler, uv + float2(-t.x,  t.y)).rgb;
    color += sourceTexture.Sample(linearSampler, uv + float2( t.x,  t.y)).rgb;
    color *= (1.0f / 16.0f);

    return float4(color * tint * intensity, 1.0f);
}

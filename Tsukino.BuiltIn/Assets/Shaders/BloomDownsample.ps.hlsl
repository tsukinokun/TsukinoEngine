//--------------------------------------------------------------
//! @file   BloomDownsample.ps.hlsl
//! @brief  ブルームの縮小（1つ前の段 → 半分の段）
//! @author 山﨑愛
//--------------------------------------------------------------
#include "Bloom.hlsli"

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(Downsample13(ToTextureUV(input.uv)), 1.0f);
}

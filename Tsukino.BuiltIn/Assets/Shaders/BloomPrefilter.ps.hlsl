//--------------------------------------------------------------
//! @file   BloomPrefilter.ps.hlsl
//! @brief  ブルームの明部抽出（シーン → 1/2 の段）
//! @author 山﨑愛
//--------------------------------------------------------------
#include "Bloom.hlsli"

//! @brief 1画素の明るさの上限。極端に明るい1画素（スペキュラの点など）が
//!        縮小段で大きな光の円に化けてちらつくのを抑える
static const float kMaxBrightness = 32.0f;

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 color = Downsample13(ToTextureUV(input.uv));
    color = min(color, kMaxBrightness);

    //----------------------------------------------------------
    // ソフトニー付きの閾値。閾値ちょうどで急に光り始めると
    // 輪郭がくっきり切れるので、knee の幅で二次曲線につなぐ
    //----------------------------------------------------------
    float brightness = max(color.r, max(color.g, color.b));
    float knee       = max(threshold * softKnee, 1e-5f);
    float soft       = clamp(brightness - threshold + knee, 0.0f, 2.0f * knee);
    soft             = soft * soft / (4.0f * knee);
    float weight     = max(soft, brightness - threshold) / max(brightness, 1e-5f);

    return float4(color * weight, 1.0f);
}

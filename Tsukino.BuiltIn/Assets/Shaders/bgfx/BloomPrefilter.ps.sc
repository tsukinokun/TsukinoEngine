$input v_texcoord0

//--------------------------------------------------------------
//! @file   BloomPrefilter.ps.sc
//! @brief  ブルームの明部抽出（シーン → 1/2 の段）
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Bloom.sh"

// 1画素の明るさの上限。極端に明るい1画素（スペキュラの点など）が
// 縮小段で大きな光の円に化けてちらつくのを抑える
#define kMaxBrightness 32.0

void main()
{
    vec3 color = Downsample13(ToTextureUV(v_texcoord0));
    color      = min(color, vec3_splat(kMaxBrightness));

    //----------------------------------------------------------
    // ソフトニー付きの閾値。閾値ちょうどで急に光り始めると
    // 輪郭がくっきり切れるので、knee の幅で二次曲線につなぐ
    //----------------------------------------------------------
    float brightness = max(color.r, max(color.g, color.b));
    float knee       = max(bloomThreshold * bloomSoftKnee, 1e-5);
    float soft       = clamp(brightness - bloomThreshold + knee, 0.0, 2.0 * knee);
    soft             = soft * soft / (4.0 * knee);
    float weight     = max(soft, brightness - bloomThreshold) / max(brightness, 1e-5);

    gl_FragColor = vec4(color * weight, 1.0);
}

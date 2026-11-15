$input v_texcoord0

//--------------------------------------------------------------
//! @file   BloomUpsample.ps.sc
//! @brief  ブルームの拡大（小さい段 → 1つ大きい段、最後はシーンへ。BloomUpsample.ps.hlsl の bgfx 版）
//! @note   書き込み先へは加算ブレンドで足す。途中の段では tint=1, intensity=1 で、
//!         シーンへ足す最後の1回だけ BloomComponent の色と強さが入る
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Bloom.sh"

void main()
{
    vec2 uv = ToTextureUV(v_texcoord0);
    vec2 t  = bloomTexelSize * bloomRadius;

    //----------------------------------------------------------
    // 3x3 のテントフィルタ（重み 1-2-1 / 2-4-2 / 1-2-1）
    //----------------------------------------------------------
    vec3 color = texture2D(s_bloomSource, uv).rgb * 4.0;
    color += texture2D(s_bloomSource, uv + vec2(-t.x, 0.0)).rgb * 2.0;
    color += texture2D(s_bloomSource, uv + vec2( t.x, 0.0)).rgb * 2.0;
    color += texture2D(s_bloomSource, uv + vec2(0.0, -t.y)).rgb * 2.0;
    color += texture2D(s_bloomSource, uv + vec2(0.0,  t.y)).rgb * 2.0;
    color += texture2D(s_bloomSource, uv + vec2(-t.x, -t.y)).rgb;
    color += texture2D(s_bloomSource, uv + vec2( t.x, -t.y)).rgb;
    color += texture2D(s_bloomSource, uv + vec2(-t.x,  t.y)).rgb;
    color += texture2D(s_bloomSource, uv + vec2( t.x,  t.y)).rgb;
    color *= (1.0 / 16.0);

    gl_FragColor = vec4(color * bloomTint * bloomIntensity, 1.0);
}

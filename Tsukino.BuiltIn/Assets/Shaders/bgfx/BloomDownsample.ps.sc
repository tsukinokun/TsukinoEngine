$input v_texcoord0

//--------------------------------------------------------------
//! @file   BloomDownsample.ps.sc
//! @brief  ブルームの縮小（1つ前の段 → 半分の段。BloomDownsample.ps.hlsl の bgfx 版）
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Bloom.sh"

void main()
{
    gl_FragColor = vec4(Downsample13(ToTextureUV(v_texcoord0)), 1.0);
}

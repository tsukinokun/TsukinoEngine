$input v_texcoord0

//--------------------------------------------------------------
//! @file   Tonemap.ps.sc
//! @brief  HDR の描画結果をトーンマップしてバックバッファへ書くピクセルシェーダー
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Tonemap.sh"

SAMPLER2D(s_hdr, 0);

void main()
{
    vec2 uv       = vec2(v_texcoord0.x, 1.0 - v_texcoord0.y);
    vec4 hdrColor = texture2D(s_hdr, uv);
    gl_FragColor  = vec4(TonemapToDisplay(hdrColor.rgb), 1.0);
}

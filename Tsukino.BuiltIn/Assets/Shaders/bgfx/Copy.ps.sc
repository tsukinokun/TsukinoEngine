$input v_texcoord0

//--------------------------------------------------------------
//! @file   Copy.ps.sc
//! @brief  テクスチャをそのまま写すピクセルシェーダー（bgfx 版だけ。頂点シェーダーは Tonemap.vs）
//! @note   エフェクトを重ねた絵（3D を描く大きさ）を、バックバッファへ引き伸ばして写すのに使う。
//!         UV は Tonemap.vs と同じく左下が (0, 0) なので、上下を戻して読む
//--------------------------------------------------------------
#include <bgfx_shader.sh>

SAMPLER2D(s_source, 0);

void main()
{
    gl_FragColor = texture2D(s_source, vec2(v_texcoord0.x, 1.0 - v_texcoord0.y));
}

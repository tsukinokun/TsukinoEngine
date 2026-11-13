$input v_color0, v_texcoord0

//--------------------------------------------------------------
//! @file   SpriteBatch.ps.sc
//! @brief  スプライトの並び（DrawKind::Sprites。文字など）用のピクセルシェーダー
//! @note   テクスチャに頂点の色を掛ける（DirectXTK の SpriteBatch と同じ）
//--------------------------------------------------------------
#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);

void main()
{
    gl_FragColor = texture2D(s_texture, v_texcoord0) * v_color0;
}

$input a_position, a_color0, a_texcoord0
$output v_color0, v_texcoord0

//--------------------------------------------------------------
//! @file   SpriteBatch.vs.sc
//! @brief  スプライトの並び（DrawKind::Sprites。文字など）用の頂点シェーダー
//! @note   Renderer が四角の頂点を CPU でクリップ空間まで作って渡すので、ここではそのまま通す
//--------------------------------------------------------------
#include <bgfx_shader.sh>

void main()
{
    gl_Position = vec4(a_position.xy, 0.0, 1.0);
    v_color0    = a_color0;
    v_texcoord0 = a_texcoord0;
}

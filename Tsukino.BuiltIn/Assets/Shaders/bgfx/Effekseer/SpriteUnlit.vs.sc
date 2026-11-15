$input a_position, a_color0, a_texcoord0
$output v_color, v_uv, v_posP

//--------------------------------------------------------------
//! @file   SpriteUnlit.vs.sc
//! @brief  Effekseer のスプライトの頂点シェーダー（sprite_unlit_vs.fx の bgfx 版）
//--------------------------------------------------------------
#define EFK_IN_UV a_texcoord0

#include <bgfx_shader.sh>
#include "EfkSpriteVS.sh"

void main()
{
#include "EfkSpriteVSBody.sh"
}

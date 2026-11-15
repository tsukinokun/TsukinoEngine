$input a_position, a_color0, a_color1, a_color2, a_texcoord0
$output v_color, v_uv, v_projBinormal, v_projTangent, v_posP

//--------------------------------------------------------------
//! @file   SpriteDistortion.vs.sc
//! @brief  Effekseer のスプライトの頂点シェーダー（sprite_distortion_vs.fx の bgfx 版）
//--------------------------------------------------------------
#define EFK_DISTORTION
#define EFK_IN_UV a_texcoord0
#define EFK_IN_NORMAL a_color1
#define EFK_IN_TANGENT a_color2

#include <bgfx_shader.sh>
#include "EfkSpriteVS.sh"

void main()
{
#include "EfkSpriteVSBody.sh"
}

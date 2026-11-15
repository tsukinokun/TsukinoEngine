$input a_position, a_color0, a_color1, a_color2, a_texcoord0, a_texcoord2, a_texcoord3, a_texcoord4, a_texcoord5, a_texcoord6
$output v_color, v_uv, v_projBinormal, v_projTangent, v_posP, v_alphaDistUV, v_blendAlphaDistUV, v_blendNextUV

//--------------------------------------------------------------
//! @file   SpriteAdDistortion.vs.sc
//! @brief  Effekseer のスプライトの頂点シェーダー（ad_sprite_distortion_vs.fx の bgfx 版）
//--------------------------------------------------------------
#define EFK_ADVANCED
#define EFK_DISTORTION
#define EFK_IN_UV a_texcoord0
#define EFK_IN_NORMAL a_color1
#define EFK_IN_TANGENT a_color2
#define EFK_IN_ALPHA_DIST_UV a_texcoord2
#define EFK_IN_BLEND_UV a_texcoord3
#define EFK_IN_BLEND_ALPHA_DIST_UV a_texcoord4
#define EFK_IN_FLIPBOOK_INDEX a_texcoord5
#define EFK_IN_ALPHA_THRESHOLD a_texcoord6

#include <bgfx_shader.sh>
#include "EfkSpriteVS.sh"

void main()
{
#include "EfkSpriteVSBody.sh"
}

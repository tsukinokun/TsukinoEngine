$input a_position, a_color0, a_color1, a_color2, a_color3, a_texcoord0
$output v_color, v_uv, v_worldN, v_alphaDistUV, v_blendAlphaDistUV, v_blendNextUV, v_posP

//--------------------------------------------------------------
//! @file   ModelAdUnlit.vs.sc
//! @brief  Effekseer のモデルの頂点シェーダー（ad_model_unlit_vs.fx の bgfx 版）
//--------------------------------------------------------------
#define EFK_ADVANCED

#include <bgfx_shader.sh>
#include "EfkModelVS.sh"

void main()
{
#include "EfkModelVSBody.sh"
}

$input a_position, a_color0, a_color1, a_color2, a_color3, a_texcoord0
$output v_color, v_uv, v_worldN, v_worldB, v_worldT, v_posP

//--------------------------------------------------------------
//! @file   ModelLit.vs.sc
//! @brief  Effekseer のモデルの頂点シェーダー（model_lit_vs.fx の bgfx 版）
//--------------------------------------------------------------
#define EFK_LIGHTING

#include <bgfx_shader.sh>
#include "EfkModelVS.sh"

void main()
{
#include "EfkModelVSBody.sh"
}

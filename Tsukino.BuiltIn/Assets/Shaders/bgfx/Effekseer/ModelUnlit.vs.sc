$input a_position, a_color0, a_color1, a_color2, a_color3, a_texcoord0
$output v_color, v_uv, v_posP

//--------------------------------------------------------------
//! @file   ModelUnlit.vs.sc
//! @brief  Effekseer のモデルの頂点シェーダー（model_unlit_vs.fx の bgfx 版）
//--------------------------------------------------------------

#include <bgfx_shader.sh>
#include "EfkModelVS.sh"

void main()
{
#include "EfkModelVSBody.sh"
}

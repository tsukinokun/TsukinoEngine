$input a_position, a_color0, a_color1, a_color2, a_color3, a_texcoord0
$output v_color, v_uv, v_projBinormal, v_projTangent, v_posP

//--------------------------------------------------------------
//! @file   ModelDistortion.vs.sc
//! @brief  Effekseer のモデルの頂点シェーダー（model_distortion_vs.fx の bgfx 版）
//--------------------------------------------------------------
#define EFK_DISTORTION

#include <bgfx_shader.sh>
#include "EfkModelVS.sh"

void main()
{
#include "EfkModelVSBody.sh"
}

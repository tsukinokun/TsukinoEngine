$input a_position, a_color0
$output v_color0

//--------------------------------------------------------------
//! @file   DebugLine.vs.sc
//! @brief  線（DrawKind::Lines）用の頂点シェーダー（DebugLine.vs.hlsl の bgfx 版）
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"

void main()
{
    gl_Position = mul(vec4(a_position, 1.0), viewProj);
    v_color0    = a_color0;
}

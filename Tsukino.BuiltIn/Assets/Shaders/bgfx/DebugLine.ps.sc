$input v_color0

//--------------------------------------------------------------
//! @file   DebugLine.ps.sc
//! @brief  線（DrawKind::Lines）用のピクセルシェーダー（DebugLine.ps.hlsl の bgfx 版）
//--------------------------------------------------------------
#include <bgfx_shader.sh>

void main()
{
    gl_FragColor = v_color0;
}

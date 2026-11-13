$input v_worldPos, v_normal, v_texcoord0, v_curClip, v_prevClip

//--------------------------------------------------------------
//! @file   Model.ps.sc
//! @brief  モデルをフォワードで描くピクセルシェーダー（Model.ps.hlsl の bgfx 版。フェード中の半透明）
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "ForwardModel.sh"

void main()
{
    gl_FragColor = ShadeForwardModel(v_worldPos, v_normal, v_texcoord0);
}

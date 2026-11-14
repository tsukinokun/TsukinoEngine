$input v_worldPos, v_normal, v_texcoord0, v_curClip, v_prevClip

//--------------------------------------------------------------
//! @file   ScreenModel.ps.sc
//! @brief  UI の層に描くモデルのピクセルシェーダー（ScreenModel.ps.hlsl の bgfx 版）
//! @note   トーンマップの後に描かれるので、ここでトーンマップまで掛ける
//--------------------------------------------------------------
#include <bgfx_shader.sh>

// UI の層はシーンの影を受けない（ScreenModel.ps.hlsl と同じ）
#define FORWARD_MODEL_NO_SHADOW
#include "ForwardModel.sh"
#include "Tonemap.sh"

void main()
{
    vec4 hdrColor = ShadeForwardModel(v_worldPos, v_normal, v_texcoord0);
    gl_FragColor  = vec4(TonemapToDisplay(hdrColor.rgb), hdrColor.a);
}

$input a_position, a_texcoord0
$output v_texcoord0, v_worldPos

//--------------------------------------------------------------
//! @file   Sprite3D.vs.sc
//! @brief  ワールド空間のスプライト用の頂点シェーダー（Sprite3D.vs.hlsl の bgfx 版）
//! @note   Sprite.vs.sc との違いは、Y座標を反転しないことだけ
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"
#include "Transform.sh"

void main()
{
    vec4 worldPos = mul(vec4(a_position, 1.0), World);
    gl_Position   = mul(worldPos, viewProj);
    v_worldPos    = worldPos.xyz;
    v_texcoord0   = vec2(a_texcoord0.x, 1.0 - a_texcoord0.y);
}

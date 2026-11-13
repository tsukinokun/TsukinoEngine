$input a_position, a_normal, a_texcoord0
$output v_worldPos, v_normal, v_texcoord0, v_curClip, v_prevClip

//--------------------------------------------------------------
//! @file   ModelStatic.vs.sc
//! @brief  スキニングの無いモデル用の頂点シェーダー（ModelStatic.vs.hlsl の bgfx 版）
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"
#include "Transform.sh"

void main()
{
    vec4 localPos = vec4(a_position, 1.0);
    vec4 worldPos = mul(localPos, world);

    v_worldPos  = worldPos.xyz;
    v_curClip   = mul(worldPos, viewProj);
    gl_Position = v_curClip;

    // HLSL 版の mul(normal, (float3x3)world)。行ベクトルなので、行列の左上 3×3 の行を重ねる
    mat4 w   = world;
    v_normal = normalize(mul(vec4(a_normal, 0.0), w).xyz);
    v_texcoord0 = a_texcoord0;

    if(motionFlags.x > 0.5) {
        vec4 prevWorldPos = mul(localPos, prevWorld);
        v_prevClip        = mul(prevWorldPos, prevViewProj);
    } else {
        v_prevClip = v_curClip;
    }
}

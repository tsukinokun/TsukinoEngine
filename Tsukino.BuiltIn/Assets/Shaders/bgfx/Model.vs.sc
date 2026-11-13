$input a_position, a_normal, a_texcoord0, a_indices, a_weight
$output v_worldPos, v_normal, v_texcoord0, v_curClip, v_prevClip

//--------------------------------------------------------------
//! @file   Model.vs.sc
//! @brief  スキニングするモデル用の頂点シェーダー（Model.vs.hlsl の bgfx 版）
//! @note   ボーン行列（DX11 版の b3 / b6。C++ の CBufferSkinning）は vec4 の配列 u_bones / u_bonesPrev。
//!         ボーン i の行列は [4i]〜[4i + 3] の4行。ボーンの番号とウェイトは2本目の頂点バッファから届く
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"
#include "Transform.sh"

#define TSUKINO_MAX_BONES 128

uniform vec4 u_bones[TSUKINO_MAX_BONES * 4];
uniform vec4 u_bonesPrev[TSUKINO_MAX_BONES * 4];

//--------------------------------------------------------------
//! @brief  ボーンの番号から、そのボーンの行列を組み立てる
//--------------------------------------------------------------
mat4 Bone(int index)
{
    return mtxFromRows(u_bones[index * 4 + 0], u_bones[index * 4 + 1], u_bones[index * 4 + 2], u_bones[index * 4 + 3]);
}

mat4 BonePrev(int index)
{
    return mtxFromRows(u_bonesPrev[index * 4 + 0], u_bonesPrev[index * 4 + 1], u_bonesPrev[index * 4 + 2], u_bonesPrev[index * 4 + 3]);
}

void main()
{
    ivec4 indices = ivec4(a_indices + 0.5);

    mat4 skinMatrix = Bone(indices.x) * a_weight.x + Bone(indices.y) * a_weight.y + Bone(indices.z) * a_weight.z + Bone(indices.w) * a_weight.w;

    vec4 localPos   = vec4(a_position, 1.0);
    vec4 skinnedPos = mul(localPos, skinMatrix);
    vec4 worldPos   = mul(skinnedPos, world);

    v_worldPos  = worldPos.xyz;
    v_curClip   = mul(worldPos, viewProj);
    gl_Position = v_curClip;

    vec3 skinnedNormal = mul(vec4(a_normal, 0.0), skinMatrix).xyz;
    v_normal           = normalize(mul(vec4(skinnedNormal, 0.0), world).xyz);
    v_texcoord0        = a_texcoord0;

    if(motionFlags.x > 0.5) {
        mat4 prevSkinMatrix = BonePrev(indices.x) * a_weight.x + BonePrev(indices.y) * a_weight.y + BonePrev(indices.z) * a_weight.z
                              + BonePrev(indices.w) * a_weight.w;
        vec4 prevSkinnedPos = mul(localPos, prevSkinMatrix);
        vec4 prevWorldPos   = mul(prevSkinnedPos, prevWorld);
        v_prevClip          = mul(prevWorldPos, prevViewProj);
    } else {
        v_prevClip = v_curClip;
    }
}

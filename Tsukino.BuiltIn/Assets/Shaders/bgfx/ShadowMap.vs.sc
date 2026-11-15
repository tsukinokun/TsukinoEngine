$input a_position, a_texcoord0, a_indices, a_weight
$output v_texcoord0

//--------------------------------------------------------------
//! @file   ShadowMap.vs.sc
//! @brief  シャドウマップへ描く頂点シェーダー（スキニングあり）
//! @note   ボーン行列は Model.vs.sc と同じ u_bones。ボーンの番号とウェイトは2本目の頂点バッファから届く。
//!         シャドウパスが描いているカスケードの番号は shadowParams.z に入っている
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"
#include "Transform.sh"

#define TSUKINO_MAX_BONES 128

uniform vec4 u_bones[TSUKINO_MAX_BONES * 4];

//--------------------------------------------------------------
//! @brief  ボーンの番号から、そのボーンの行列を組み立てる
//--------------------------------------------------------------
mat4 Bone(int index)
{
    return mtxFromRows(u_bones[index * 4 + 0], u_bones[index * 4 + 1], u_bones[index * 4 + 2], u_bones[index * 4 + 3]);
}

void main()
{
    ivec4 indices = ivec4(a_indices + 0.5);

    mat4 skinMatrix = Bone(indices.x) * a_weight.x + Bone(indices.y) * a_weight.y + Bone(indices.z) * a_weight.z + Bone(indices.w) * a_weight.w;

    vec4 skinnedPos = mul(vec4(a_position, 1.0), skinMatrix);
    vec4 worldPos   = mul(skinnedPos, world);
    int  cascade    = int(shadowParams.z + 0.5);

    gl_Position = mul(worldPos, cascadeViewProj(cascade));
    v_texcoord0 = a_texcoord0;    // くり抜きの影のため、ピクセルシェーダーへ UV も渡す
}

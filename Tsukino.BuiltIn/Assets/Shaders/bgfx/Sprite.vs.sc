$input a_position, a_texcoord0
$output v_texcoord0, v_worldPos

//--------------------------------------------------------------
//! @file   Sprite.vs.sc
//! @brief  画面のスプライト用の頂点シェーダー（Sprite.vs.hlsl の bgfx 版）
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"
#include "Transform.sh"

void main()
{
    // 頂点 * ワールド行列 * ビュープロジェクション行列
    vec4 position = mul(vec4(a_position, 1.0), mul(World, viewProj));

    // Y座標を反転（HLSL 版と同じ。UI のカメラは上向きが +Y の正射影で作られている）
    position.y = -position.y;
    gl_Position = position;

    v_texcoord0 = vec2(a_texcoord0.x, 1.0 - a_texcoord0.y);
    v_worldPos  = vec3_splat(0.0);    // Sprite.ps.sc の入力と揃えるためだけに出す
}

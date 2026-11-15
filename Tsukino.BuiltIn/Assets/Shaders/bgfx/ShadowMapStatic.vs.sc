$input a_position, a_texcoord0
$output v_texcoord0

//--------------------------------------------------------------
//! @file   ShadowMapStatic.vs.sc
//! @brief  シャドウマップへ描く頂点シェーダー（スキニング無し）
//! @note   シャドウパスが描いているカスケードの番号は shadowParams.z に入っている。
//!         こうしておくと、このシェーダーはカスケードの枚数を知らなくて済む
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"
#include "Transform.sh"

void main()
{
    vec4 worldPos = mul(vec4(a_position, 1.0), world);
    int  cascade  = int(shadowParams.z + 0.5);

    gl_Position = mul(worldPos, cascadeViewProj(cascade));
    v_texcoord0 = a_texcoord0;    // くり抜きの影のため、ピクセルシェーダーへ UV も渡す
}

$input v_worldPos, v_normal, v_texcoord0, v_curClip, v_prevClip

//--------------------------------------------------------------
//! @file   RecipeColor.ps.sc
//! @brief  レシピ: CustomShaderComponent::params[0] の色で塗るピクセルシェーダー
//! @note   入力は組み込みの頂点シェーダー（ModelStatic.vs.sc）の出力と同じ並びにする。
//!         CustomShaderComponent::params は u_user0[0]〜[3] に届く。
//!         経過時間が要るなら #include "Scene.sh" して timeParams.x（秒）を読む
//--------------------------------------------------------------
#include <bgfx_shader.sh>

uniform vec4 u_user0[4];    // CustomShaderComponent::params

void main()
{
    // ライティングは掛からないので、向きで少しだけ陰影を付けて立体に見せる
    vec3  toLight = normalize(vec3(-0.4, 1.0, -0.6));
    float shade   = 0.6 + 0.4 * max(dot(normalize(v_normal), toLight), 0.0);
    gl_FragColor  = vec4(u_user0[0].rgb * shade, 1.0);
}

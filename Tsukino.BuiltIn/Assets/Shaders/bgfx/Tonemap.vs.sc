$input a_position, a_texcoord0
$output v_texcoord0

//--------------------------------------------------------------
//! @file   Tonemap.vs.sc
//! @brief  フルスクリーン三角形の頂点シェーダー（Tonemap.vs.hlsl の bgfx 版。後処理・ライティングで共用）
//! @note   HLSL 版は SV_VertexID から三角形を作っていた。bgfx 版は C++（FullscreenPass）が
//!         3つの頂点を一時バッファで渡す。UV は HLSL 版と同じく左下が (0, 0)
//--------------------------------------------------------------
#include <bgfx_shader.sh>

void main()
{
    gl_Position = vec4(a_position.xy, 0.0, 1.0);    // リバース Z：z = 0 が最も遠い
    v_texcoord0 = a_texcoord0;
}

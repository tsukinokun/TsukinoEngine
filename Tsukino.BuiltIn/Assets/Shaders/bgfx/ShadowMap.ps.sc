$input v_texcoord0

//--------------------------------------------------------------
//! @file   ShadowMap.ps.sc
//! @brief  シャドウマップへ描くピクセルシェーダー（ShadowMap.ps.hlsl の bgfx 版）
//! @note   深度はハードウェアが書く。描く先に色のバッファは無いので、色は捨てられる。
//!         やることは画素を抜くことだけ。
//!         - くり抜き（アルファテスト）：マテリアルの alphaCutoff が 0 より大きいとき、
//!           アルベドのアルファで形を抜く（GBuffer.ps.sc と同じ判定にして、見た目と影の形を揃える）
//!         - 網点フェード：motionFlags.y で画素を抜き、物が少しずつ出るときに影も一緒に現れるようにする
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Transform.sh"
#include "Material.sh"
#include "Dither.sh"
#include "TextureStages.sh"

SAMPLER2D(s_albedo, STAGE_ALBEDO);

void main()
{
    ClipDither(gl_FragCoord.xy, motionFlags.y);

    if(alphaCutoff > 0.0 && texture2D(s_albedo, v_texcoord0).a - alphaCutoff < 0.0)
        discard;

    gl_FragColor = vec4_splat(0.0);
}

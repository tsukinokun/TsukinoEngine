$input v_texcoord0, v_color0

//--------------------------------------------------------------
//! @file   AmbientParticle.ps.sc
//! @brief  環境パーティクル用のピクセルシェーダー（AmbientParticle.ps.hlsl の bgfx 版）
//! @note   板の中心からの距離だけで丸い減衰を作る。テクスチャも定数も使わない
//!         （色・輝度・フェードは頂点シェーダーが計算し、補間値として受け取る）。
//!         ブレンドは SRC_ALPHA / ONE の加算合成を前提にしているので、色に事前乗算はしない
//--------------------------------------------------------------
#include <bgfx_shader.sh>

void main()
{
    // 中心からの距離で丸く抜く。四隅（距離約1.41）は完全に0になるので、板の輪郭が四角く見えることはない
    float r = length(v_texcoord0);

    // 外周へ向かってなめらかに0へ。二乗して中心に芯を残す
    float falloff = 1.0 - smoothstep(0.0, 1.0, r);
    falloff *= falloff;

    gl_FragColor = vec4(v_color0.rgb, v_color0.a * falloff);
}

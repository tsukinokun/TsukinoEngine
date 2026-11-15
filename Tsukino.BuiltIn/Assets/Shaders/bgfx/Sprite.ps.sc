$input v_texcoord0, v_worldPos

//--------------------------------------------------------------
//! @file   Sprite.ps.sc
//! @brief  スプライト用のピクセルシェーダー（Sprite.ps.hlsl の bgfx 版）
//! @note   ワールド空間のスプライトへは、自分の距離ぶんの霧をここで掛ける（カメラ位置は Scene.sh、霧の定数は Fog.sh）
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"
#include "Material.sh"
#include "Fog.sh"

SAMPLER2D(s_albedo, 0);    // テクスチャ（DX11 版の t0 / s0）

//--------------------------------------------------------------
//! @brief  左右・上下の塗りで、このピクセルを塗る割合を求める
//! @param  uv [in] テクスチャ座標（画面上で x が右、y が下）
//! @return 0〜1。塗らない側は 0、境目は1ピクセル幅でなめらかに変わる
//--------------------------------------------------------------
float LinearFillCoverage(vec2 uv)
{
    // 塗り始めの端からの位置（0〜1）。左右は左端から、上下は下端から測り、spriteFill.z が 1 なら逆の端から測る
    float t  = (spriteFill.x < 2.5) ? uv.x : 1.0 - uv.y;
    t        = mix(t, 1.0 - t, spriteFill.z);
    float aa = max(fwidth(t), 1.0e-5);
    return saturate((spriteFill.y - t) / aa + 0.5);
}

//--------------------------------------------------------------
//! @brief  塗り（SpriteFillMode）で、このピクセルを塗る割合を求める
//! @param  uv [in] テクスチャ座標（画面上で x が右、y が下）
//! @return 0〜1。塗りなしのときは常に 1
//--------------------------------------------------------------
float FillCoverage(vec2 uv)
{
    const float kTwoPi = 6.28318530718;
    if(spriteFill.x < 0.5 || spriteFill.y >= 1.0)
        return 1.0;
    if(spriteFill.x > 1.5)
        return LinearFillCoverage(uv);

    // 中心から見た角度を、真上を 0 として時計回りに測る
    vec2  d     = uv - 0.5;
    float angle = atan2(d.x, -d.y);

    // 開始角から塗る向きに何割進んだ位置か（0〜1）
    float t = fract(((angle - spriteFill.z) * spriteFill.w) / kTwoPi + 1.0);

    // 1ピクセルあたりの割合の変化量。角度は中心に近いほど速く変わるので、半径で割って求める
    float aa = length(fwidth(d)) / (max(length(d), 1.0e-4) * kTwoPi);
    return saturate((spriteFill.y - t) / max(aa, 1.0e-5) + 0.5);
}

void main()
{
    // 描く四角の UV を画像の一部へ当てる（9 スライスの1枚ずつ）。塗りは当てる前の UV で測る
    vec2 textureUv = spriteUv.xy + v_texcoord0 * spriteUv.zw;
    vec4 color     = texture2D(s_albedo, textureUv) * baseColor;
    color.a *= FillCoverage(v_texcoord0);

    //----------------------------------------------------------
    // 霧。ワールド空間のスプライトは半透明のパス（フォグの後）で描かれ、その描画にだけ
    // フォワード用のフォグ定数（distanceParams.w = 1）が届く（FogPass::BindForwardFog）。
    // 深度を書かないので、フォグパスに任せると奥の景色の距離で霧が掛かってしまう。ここで自分の距離ぶんだけ掛ける。
    // 半透明は霧の色へ寄せ、加算は光そのものを弱める（加算で霧の色を足すと白く浮くため）。
    // 画面のスプライト（Overlay）では 0 が届き、何もしない
    //----------------------------------------------------------
    if(distanceParams.w > 0.5) {
        vec4 fog = ComputeFog(v_worldPos, normalize(v_worldPos - cameraPos.xyz), false);
        if(spriteFog.x > 0.5)
            color.rgb *= 1.0 - fog.a;
        else
            color.rgb = mix(color.rgb, fog.rgb, fog.a);
    }
    gl_FragColor = color;
}

//--------------------------------------------------------------
//! @file   Dither.sh
//! @brief  4×4 の網点で画素を抜く（Dither.hlsli の bgfx 版）
//--------------------------------------------------------------
#ifndef TSUKINO_DITHER_SH
#define TSUKINO_DITHER_SH

//--------------------------------------------------------------
//! @brief  4×4 のベイヤー行列のしきい値（0〜1）を返す
//! @param  pixel [in] 画素の位置（gl_FragCoord.xy）
//! @note   表を持たず、2×2 の行列 {0,2 / 3,1} を2段重ねて求める（HLSL 版の表と同じ値になる）
//--------------------------------------------------------------
float BayerThreshold4x4(vec2 pixel)
{
    vec2  p      = mod(floor(pixel), 4.0);
    vec2  low    = mod(p, 2.0);
    vec2  high   = floor(p / 2.0);
    float bLow   = low.x * 2.0 + low.y * 3.0 - 4.0 * low.x * low.y;
    float bHigh  = high.x * 2.0 + high.y * 3.0 - 4.0 * high.x * high.y;
    return (bLow * 4.0 + bHigh + 0.5) / 16.0;
}

//--------------------------------------------------------------
//! @brief  見えている割合 fade に応じて、網点でこの画素を抜く（1 以上なら何もしない）
//--------------------------------------------------------------
void ClipDither(vec2 pixel, float fade)
{
    if(fade < 1.0 && fade - BayerThreshold4x4(pixel) < 0.0)
        discard;
}

#endif    // TSUKINO_DITHER_SH

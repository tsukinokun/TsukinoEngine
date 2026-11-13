//--------------------------------------------------------------
//! @file   Tonemap.sh
//! @brief  HDR から画面の色へのトーンマップ（Tonemap.hlsli の bgfx 版）
//--------------------------------------------------------------
#ifndef TSUKINO_TONEMAP_SH
#define TSUKINO_TONEMAP_SH

#define kExposure 0.6

//--------------------------------------------------------------
//! @brief  ACES のフィルミック曲線（近似）
//--------------------------------------------------------------
vec3 ACES(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

//--------------------------------------------------------------
//! @brief  HDR の色を画面の色にする
//--------------------------------------------------------------
vec3 TonemapToDisplay(vec3 hdrColor)
{
    return ACES(hdrColor * kExposure);
}

#endif    // TSUKINO_TONEMAP_SH

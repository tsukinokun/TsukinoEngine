//--------------------------------------------------------------
//! @file   EfkDistortionPS.sh
//! @brief  Effekseer の歪みのピクセルシェーダーの定数（Distortion_Common_PS.fx の bgfx 版）
//! @note   C++ の EffekseerRenderer::PixelConstantBufferDistortion を vec4 の配列 u_efkDistPS で受け取る
//--------------------------------------------------------------
#ifndef TSUKINO_EFK_DISTORTION_PS_SH
#define TSUKINO_EFK_DISTORTION_PS_SH

#include "EfkCommon.sh"

uniform vec4 u_efkDistPS[8];

#define g_scale                u_efkDistPS[0]    // x: 歪みの強さ
#define mUVInversedBack        u_efkDistPS[1]
#define fFlipbookParameter     u_efkDistPS[2]
#define fUVDistortionParameter u_efkDistPS[3]
#define fBlendTextureParameter u_efkDistPS[4]
#define softParticleParam      u_efkDistPS[5]
#define reconstructionParam1   u_efkDistPS[6]
#define reconstructionParam2   u_efkDistPS[7]

#endif    // TSUKINO_EFK_DISTORTION_PS_SH

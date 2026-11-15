//--------------------------------------------------------------
//! @file   EfkPS.sh
//! @brief  Effekseer の unlit / lit のピクセルシェーダーの定数（model_ps.fx・ad_model_ps.fx の bgfx 版）
//! @note   C++ の EffekseerRenderer::PixelConstantBuffer を vec4 の配列 u_efkPS で受け取る
//--------------------------------------------------------------
#ifndef TSUKINO_EFK_PS_SH
#define TSUKINO_EFK_PS_SH

#include "EfkCommon.sh"

uniform vec4 u_efkPS[18];

#define fLightDirection        u_efkPS[0]
#define fLightColor            u_efkPS[1]
#define fLightAmbient          u_efkPS[2]
#define fFlipbookParameter     u_efkPS[3]     // x: 補間するか、y: 補間の種類
#define fUVDistortionParameter u_efkPS[4]     // x: 強さ、y: ブレンドの強さ、zw: 上下の反転
#define fBlendTextureParameter u_efkPS[5]     // x: ブレンドの種類
#define fCameraFrontDirection  u_efkPS[6]
#define fFalloffParameter      u_efkPS[7]     // x: 有効か、y: 色の合成の種類、z: 指数
#define fFalloffBeginColor     u_efkPS[8]
#define fFalloffEndColor       u_efkPS[9]
#define fEmissiveScaling       u_efkPS[10]    // x: 発光の倍率
#define fEdgeColor             u_efkPS[11]
#define fEdgeParameter         u_efkPS[12]    // x: 閾値、y: 色の倍率
#define softParticleParam      u_efkPS[13]
#define reconstructionParam1   u_efkPS[14]
#define reconstructionParam2   u_efkPS[15]
#define mUVInversedBack        u_efkPS[16]
#define miscFlags              u_efkPS[17]    // x: リニアの画面へガンマのテクスチャを描くか

#endif    // TSUKINO_EFK_PS_SH

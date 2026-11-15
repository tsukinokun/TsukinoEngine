//--------------------------------------------------------------
//! @file   EfkModelVS.sh
//! @brief  Effekseer のモデルの頂点シェーダーの定数（model_vs.fx・ad_model_vs.fx の bgfx 版）
//! @note   C++ の EffekseerRenderer::ModelRendererVertexConstantBuffer<N>（advanced は
//!         ModelRendererAdvancedVertexConstantBuffer<N>）を vec4 の配列で受け取る。
//!         1回の描画でモデルを EFK_MODEL_INSTANCE_COUNT 個まで並べ（インスタンス描画）、gl_InstanceID で選ぶ。
//!         個数は C++ の kModelInstanceCount（src/Bgfx/Effekseer/ModelRenderer.cpp）と揃えること。
//!         モバイルの GPU でも頂点シェーダーの定数（vec4 で 256 個）に収まる数にしてある。
//!         本体は EfkModelVSBody.sh（main の中で include する）
//--------------------------------------------------------------
#ifndef TSUKINO_EFK_MODEL_VS_SH
#define TSUKINO_EFK_MODEL_VS_SH

#include "EfkCommon.sh"

#define EFK_MODEL_INSTANCE_COUNT 10

#if defined(EFK_ADVANCED)

uniform vec4 u_efkModelAdVS[10 + 13 * EFK_MODEL_INSTANCE_COUNT];
#define EFK_MODEL_VS u_efkModelAdVS

#define fUV(_i)                       EFK_MODEL_VS[4 + 4 * EFK_MODEL_INSTANCE_COUNT + (_i)]
#define fAlphaUV(_i)                  EFK_MODEL_VS[4 + 5 * EFK_MODEL_INSTANCE_COUNT + (_i)]
#define fUVDistortionUV(_i)           EFK_MODEL_VS[4 + 6 * EFK_MODEL_INSTANCE_COUNT + (_i)]
#define fBlendUV(_i)                  EFK_MODEL_VS[4 + 7 * EFK_MODEL_INSTANCE_COUNT + (_i)]
#define fBlendAlphaUV(_i)             EFK_MODEL_VS[4 + 8 * EFK_MODEL_INSTANCE_COUNT + (_i)]
#define fBlendUVDistortionUV(_i)      EFK_MODEL_VS[4 + 9 * EFK_MODEL_INSTANCE_COUNT + (_i)]
#define flipbookParameter1            EFK_MODEL_VS[4 + 10 * EFK_MODEL_INSTANCE_COUNT]
#define flipbookParameter2            EFK_MODEL_VS[5 + 10 * EFK_MODEL_INSTANCE_COUNT]
#define fFlipbookIndexAndNextRate(_i) EFK_MODEL_VS[6 + 10 * EFK_MODEL_INSTANCE_COUNT + (_i)]
#define fModelAlphaThreshold(_i)      EFK_MODEL_VS[6 + 11 * EFK_MODEL_INSTANCE_COUNT + (_i)]
#define fModelColor(_i)               EFK_MODEL_VS[6 + 12 * EFK_MODEL_INSTANCE_COUNT + (_i)]
#define mUVInversed                   EFK_MODEL_VS[9 + 13 * EFK_MODEL_INSTANCE_COUNT]

#else

uniform vec4 u_efkModelVS[8 + 6 * EFK_MODEL_INSTANCE_COUNT];
#define EFK_MODEL_VS u_efkModelVS

#define fUV(_i)         EFK_MODEL_VS[4 + 4 * EFK_MODEL_INSTANCE_COUNT + (_i)]
#define fModelColor(_i) EFK_MODEL_VS[4 + 5 * EFK_MODEL_INSTANCE_COUNT + (_i)]
#define mUVInversed     EFK_MODEL_VS[7 + 6 * EFK_MODEL_INSTANCE_COUNT]

#endif

#define mCameraProj mtxFromRows(EFK_MODEL_VS[0], EFK_MODEL_VS[1], EFK_MODEL_VS[2], EFK_MODEL_VS[3])
#define mModel(_i)  mtxFromRows(EFK_MODEL_VS[4 + 4 * (_i)], EFK_MODEL_VS[5 + 4 * (_i)], EFK_MODEL_VS[6 + 4 * (_i)], EFK_MODEL_VS[7 + 4 * (_i)])

#endif    // TSUKINO_EFK_MODEL_VS_SH

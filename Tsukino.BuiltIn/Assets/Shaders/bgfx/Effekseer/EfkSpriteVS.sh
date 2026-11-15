//--------------------------------------------------------------
//! @file   EfkSpriteVS.sh
//! @brief  Effekseer のスプライト・リボン・リング・軌跡の頂点シェーダーの定数（sprite_unlit_vs.fx・ad_sprite_unlit_vs.fx の bgfx 版）
//! @note   C++ の EffekseerRenderer::StandardRendererVertexBuffer を vec4 の配列 u_efkSpriteVS で受け取る。
//!         行列は行優先で並んでいるので mtxFromRows で組み立て、mul(行ベクトル, 行列) で掛ける。
//!         本体は EfkSpriteVSBody.sh（main の中で include する）
//--------------------------------------------------------------
#ifndef TSUKINO_EFK_SPRITE_VS_SH
#define TSUKINO_EFK_SPRITE_VS_SH

#include "EfkCommon.sh"

uniform vec4 u_efkSpriteVS[11];

#define mCamera            mtxFromRows(u_efkSpriteVS[0], u_efkSpriteVS[1], u_efkSpriteVS[2], u_efkSpriteVS[3])
#define mCameraProj        mtxFromRows(u_efkSpriteVS[4], u_efkSpriteVS[5], u_efkSpriteVS[6], u_efkSpriteVS[7])
#define mUVInversed        u_efkSpriteVS[8]
#define flipbookParameter1 u_efkSpriteVS[9]     // x: 補間するか、y: ループの種類、z: 横の分割数、w: 縦の分割数
#define flipbookParameter2 u_efkSpriteVS[10]    // xy: 1コマの大きさ、zw: ずれ

#endif    // TSUKINO_EFK_SPRITE_VS_SH

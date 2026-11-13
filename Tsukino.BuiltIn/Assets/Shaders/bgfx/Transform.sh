//--------------------------------------------------------------
//! @file   Transform.sh
//! @brief  物ごとの定数（DX11 版の b1。C++ の CBufferTransform）の bgfx 版
//! @note   Scene.sh と同じく、vec4 の配列 u_transform を #define で名前に当てる。
//!         HLSL 版は World（スプライト）と world（モデル）の両方の名前で読んでいるので、両方を用意する
//--------------------------------------------------------------
#ifndef TSUKINO_TRANSFORM_SH
#define TSUKINO_TRANSFORM_SH

#define TSUKINO_TRANSFORM_VEC4_COUNT 9    // sizeof(CBufferTransform) / 16

uniform vec4 u_transform[TSUKINO_TRANSFORM_VEC4_COUNT];

#define World       mtxFromRows(u_transform[0], u_transform[1], u_transform[2], u_transform[3])
#define PrevWorld   mtxFromRows(u_transform[4], u_transform[5], u_transform[6], u_transform[7])
#define world       World
#define prevWorld   PrevWorld
#define motionFlags u_transform[8]

#endif    // TSUKINO_TRANSFORM_SH

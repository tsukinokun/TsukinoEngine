//--------------------------------------------------------------
//! @file   Scene.sh
//! @brief  シーン定数（DX11 版の b0。C++ の CBufferScene）の bgfx 版
//! @note   定数バッファ1つを vec4 の配列のユニフォーム1つで受け取る。
//!         C++ 側は UniformBlock<CBufferScene> が構造体をそのまま送る（名前は u_scene）。
//!         HLSL 版（Scene.hlsli）と同じ名前で読めるよう、メンバーは #define で配列の要素へ当てる。
//!         行列は C++ と HLSL と同じく行優先で並んでいるので、mtxFromRows で組み立てる。
//!         並びを変えたら ConstantBuffer.hpp の static_assert と一緒に直すこと。
//--------------------------------------------------------------
#ifndef TSUKINO_SCENE_SH
#define TSUKINO_SCENE_SH

#define TSUKINO_SHADOW_CASCADE_COUNT 4
#define TSUKINO_SCENE_VEC4_COUNT 47    // sizeof(CBufferScene) / 16

uniform vec4 u_scene[TSUKINO_SCENE_VEC4_COUNT];

#define view                mtxFromRows(u_scene[0], u_scene[1], u_scene[2], u_scene[3])
#define projection          mtxFromRows(u_scene[4], u_scene[5], u_scene[6], u_scene[7])
#define viewProj            mtxFromRows(u_scene[8], u_scene[9], u_scene[10], u_scene[11])
#define invViewProj         mtxFromRows(u_scene[12], u_scene[13], u_scene[14], u_scene[15])
#define cascadeViewProj(_i) mtxFromRows(u_scene[16 + (_i) * 4], u_scene[17 + (_i) * 4], u_scene[18 + (_i) * 4], u_scene[19 + (_i) * 4])
#define lightDir            u_scene[32]
#define lightColor          u_scene[33]
#define cameraPos           u_scene[34]
#define prevViewProj        mtxFromRows(u_scene[35], u_scene[36], u_scene[37], u_scene[38])
#define timeParams          u_scene[39]
#define screenParams        u_scene[40]
#define shadowParams        u_scene[41]
#define cascadeTexelWorld   u_scene[42]
#define cascadeSplitFar     u_scene[43]
#define occlusionCutout0    u_scene[44]
#define occlusionCutout1    u_scene[45]
#define qualityParams       u_scene[46]

#endif    // TSUKINO_SCENE_SH

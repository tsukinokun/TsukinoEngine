//--------------------------------------------------------------
//! @file   Material.sh
//! @brief  マテリアルの定数（DX11 版の b2。C++ の CBufferMaterial）の bgfx 版
//! @note   Scene.sh と同じく、vec4 の配列 u_material を #define で名前に当てる。
//!         float1つのメンバーは、入っている vec4 の成分で読む。
//--------------------------------------------------------------
#ifndef TSUKINO_MATERIAL_SH
#define TSUKINO_MATERIAL_SH

#define TSUKINO_MATERIAL_VEC4_COUNT 10    // sizeof(CBufferMaterial) / 16

uniform vec4 u_material[TSUKINO_MATERIAL_VEC4_COUNT];

#define baseColor      u_material[0]
#define emissive       u_material[1].xyz
#define metallic       u_material[2].x
#define roughness      u_material[2].y
#define specular       u_material[2].z
#define alphaCutoff    u_material[2].w
#define rimColor       u_material[3]
#define rimParams      u_material[4]
#define spriteFill     u_material[5]
#define shading        u_material[6]
#define toonShadeColor u_material[7]
#define spriteUv       u_material[8]
#define spriteFog      u_material[9]

#endif    // TSUKINO_MATERIAL_SH

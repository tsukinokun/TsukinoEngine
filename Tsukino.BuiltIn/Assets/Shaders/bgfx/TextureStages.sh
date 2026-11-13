//--------------------------------------------------------------
//! @file   TextureStages.sh
//! @brief  bgfx 版のテクスチャの番号（ステージ）
//! @note   bgfx のテクスチャは 16 枚（0〜15）まで。DX11 版の t8〜t20 は入らないので、詰め直している。
//!         C++ 側の Tsukino/Renderer/Bgfx/TextureStages.hpp と必ず揃えること
//--------------------------------------------------------------
#ifndef TSUKINO_TEXTURE_STAGES_SH
#define TSUKINO_TEXTURE_STAGES_SH

// マテリアル（DX11 版の t0〜t4）
#define STAGE_ALBEDO             0
#define STAGE_NORMAL_MAP         1
#define STAGE_METALLIC_ROUGHNESS 2
#define STAGE_EMISSIVE_MAP       3
#define STAGE_AO_MAP             4

// 影（DX11 版の t8）
#define STAGE_SHADOW_MAP         5

// G-Buffer（DX11 版の t9〜t14）
#define STAGE_GBUFFER_ALBEDO     6
#define STAGE_GBUFFER_NORMAL     7
#define STAGE_GBUFFER_MATERIAL   8
#define STAGE_GBUFFER_EMISSIVE   9
#define STAGE_GBUFFER_DEPTH      10
#define STAGE_GBUFFER_WORLD_POS  11

// IBL（DX11 版の t17〜t19）
#define STAGE_IBL_IRRADIANCE     12
#define STAGE_IBL_SPECULAR       13
#define STAGE_IBL_BRDF_LUT       14

#endif    // TSUKINO_TEXTURE_STAGES_SH

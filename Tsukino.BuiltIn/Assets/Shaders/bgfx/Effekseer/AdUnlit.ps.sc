$input v_color, v_uv, v_worldN, v_alphaDistUV, v_blendAlphaDistUV, v_blendNextUV, v_posP

//--------------------------------------------------------------
//! @file   AdUnlit.ps.sc
//! @brief  Effekseer のピクセルシェーダー（ad_model_unlit_ps.fx の bgfx 版。スプライトとモデルが共用）
//! @note   テクスチャのステージは Effekseer が渡す並び（DX11 版のレジスタ）と同じ。
//!         サンプラーは s_efkTex0〜7 を役割の名前へ当てる
//--------------------------------------------------------------
#define EFK_ADVANCED

#include <bgfx_shader.sh>
#include "EfkPS.sh"

SAMPLER2D(s_efkTex0, 0);
SAMPLER2D(s_efkTex1, 1);
SAMPLER2D(s_efkTex2, 2);
SAMPLER2D(s_efkTex3, 3);
SAMPLER2D(s_efkTex4, 4);
SAMPLER2D(s_efkTex5, 5);
SAMPLER2D(s_efkTex6, 6);
#define s_colorTex s_efkTex0
#define s_alphaTex s_efkTex1
#define s_uvDistortionTex s_efkTex2
#define s_blendTex s_efkTex3
#define s_blendAlphaTex s_efkTex4
#define s_blendUVDistortionTex s_efkTex5
#define s_depthTex s_efkTex6

void main()
{
#include "EfkPSBody.sh"
}

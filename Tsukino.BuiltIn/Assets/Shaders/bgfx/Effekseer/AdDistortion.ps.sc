$input v_color, v_uv, v_projBinormal, v_projTangent, v_posP, v_alphaDistUV, v_blendAlphaDistUV, v_blendNextUV

//--------------------------------------------------------------
//! @file   AdDistortion.ps.sc
//! @brief  Effekseer のピクセルシェーダー（ad_model_distortion_ps.fx の bgfx 版。スプライトとモデルが共用）
//! @note   テクスチャのステージは Effekseer が渡す並び（DX11 版のレジスタ）と同じ。
//!         サンプラーは s_efkTex0〜7 を役割の名前へ当てる
//--------------------------------------------------------------
#define EFK_ADVANCED
#define EFK_DISTORTION

#include <bgfx_shader.sh>
#include "EfkDistortionPS.sh"

SAMPLER2D(s_efkTex0, 0);
SAMPLER2D(s_efkTex1, 1);
SAMPLER2D(s_efkTex2, 2);
SAMPLER2D(s_efkTex3, 3);
SAMPLER2D(s_efkTex4, 4);
SAMPLER2D(s_efkTex5, 5);
SAMPLER2D(s_efkTex6, 6);
SAMPLER2D(s_efkTex7, 7);
#define s_colorTex s_efkTex0
#define s_backTex s_efkTex1
#define s_alphaTex s_efkTex2
#define s_uvDistortionTex s_efkTex3
#define s_blendTex s_efkTex4
#define s_blendAlphaTex s_efkTex5
#define s_blendUVDistortionTex s_efkTex6
#define s_depthTex s_efkTex7

void main()
{
#include "EfkDistortionPSBody.sh"
}

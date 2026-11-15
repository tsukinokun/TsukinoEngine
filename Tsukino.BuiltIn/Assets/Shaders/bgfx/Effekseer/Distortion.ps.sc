$input v_color, v_uv, v_projBinormal, v_projTangent, v_posP

//--------------------------------------------------------------
//! @file   Distortion.ps.sc
//! @brief  Effekseer のピクセルシェーダー（model_distortion_ps.fx の bgfx 版。スプライトとモデルが共用）
//! @note   テクスチャのステージは Effekseer が渡す並び（DX11 版のレジスタ）と同じ。
//!         サンプラーは s_efkTex0〜7 を役割の名前へ当てる
//--------------------------------------------------------------
#define EFK_DISTORTION

#include <bgfx_shader.sh>
#include "EfkDistortionPS.sh"

SAMPLER2D(s_efkTex0, 0);
SAMPLER2D(s_efkTex1, 1);
SAMPLER2D(s_efkTex2, 2);
#define s_colorTex s_efkTex0
#define s_backTex s_efkTex1
#define s_depthTex s_efkTex2

void main()
{
#include "EfkDistortionPSBody.sh"
}

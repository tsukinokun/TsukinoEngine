$input v_color, v_uv, v_posP

//--------------------------------------------------------------
//! @file   Unlit.ps.sc
//! @brief  Effekseer のピクセルシェーダー（model_unlit_ps.fx の bgfx 版。スプライトとモデルが共用）
//! @note   テクスチャのステージは Effekseer が渡す並び（DX11 版のレジスタ）と同じ。
//!         サンプラーは s_efkTex0〜7 を役割の名前へ当てる
//--------------------------------------------------------------

#include <bgfx_shader.sh>
#include "EfkPS.sh"

SAMPLER2D(s_efkTex0, 0);
SAMPLER2D(s_efkTex1, 1);
#define s_colorTex s_efkTex0
#define s_depthTex s_efkTex1

void main()
{
#include "EfkPSBody.sh"
}

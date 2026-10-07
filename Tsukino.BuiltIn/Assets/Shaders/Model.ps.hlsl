//--------------------------------------------------------------
//! @file   Model.ps.hlsl
//! @brief  3Dモデル用のフォワードピクセルシェーダ
//! @author 山﨑愛
//! @note   不透明の通常描画はGBuffer.ps.hlsl（ディファード）が担当する。
//!         本シェーダーは半透明（Transparent）等、フォワードでの
//!         ライティングが必要な用途に使う。照らす式は ForwardModel.hlsli にあり、
//!         UI の層に描くモデル（ScreenModel.ps.hlsl）と共用している。
//--------------------------------------------------------------
#pragma pack_matrix(row_major)
#include "ForwardModel.hlsli"

//--------------------------------------------------------------
//! @brief ピクセルシェーダメイン関数
//--------------------------------------------------------------
float4 PSMain(PSInput input) : SV_TARGET
{
    return ShadeForwardModel(input);
}

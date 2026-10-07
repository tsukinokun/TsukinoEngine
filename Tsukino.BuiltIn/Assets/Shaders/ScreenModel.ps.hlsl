//--------------------------------------------------------------
//! @file   ScreenModel.ps.hlsl
//! @brief  画面の UI の層に描く 3D モデル用のピクセルシェーダ（ScreenModelComponent）
//! @detail UI の層はトーンマップの後に描かれるため、照らした HDR の色に、
//!         画面全体と同じトーンマップ（Tonemap.hlsli）をここで掛けます。
//!         シャドウマップは使いません（影は落ちない）。
//--------------------------------------------------------------
#pragma pack_matrix(row_major)
#define FORWARD_MODEL_NO_SHADOW
#include "ForwardModel.hlsli"
#include "Tonemap.hlsli"

//--------------------------------------------------------------
//! @brief ピクセルシェーダメイン関数
//--------------------------------------------------------------
float4 PSMain(PSInput input) : SV_TARGET
{
    float4 hdrColor = ShadeForwardModel(input);
    return float4(TonemapToDisplay(hdrColor.rgb), hdrColor.a);
}

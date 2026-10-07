//--------------------------------------------------------------
//! @file   Tonemap.ps.hlsl
//! @brief  トーンマッピング用ピクセルシェーダー
//! @author 山﨑愛
//--------------------------------------------------------------

#include "Tonemap.hlsli"

//--------------------------------------------------------------
//! @brief HDRレンダーターゲット (t0)
//--------------------------------------------------------------
Texture2D hdrTexture : register(t0);
SamplerState hdrSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief ピクセルシェーダーメイン
//--------------------------------------------------------------
float4 PSMain(PSInput input) : SV_TARGET
{
        // UV のY を反転
    float2 uv = float2(input.uv.x, 1.0f - input.uv.y);

    // HDRバッファをサンプリング
    float4 hdrColor = hdrTexture.Sample(hdrSampler, uv);

    // ACESトーンマッピング（RGBのみ。式は Tonemap.hlsli）。ワールドの絵へのトーンマップはこの1回だけで、
    // アルベドやBRDFの途中結果には掛けない（UI の層に描く 3D モデルは ScreenModel.ps.hlsl が同じ式を掛ける）
    float3 ldrColor = TonemapToDisplay(hdrColor.rgb);

    // アルファは乗算せずそのまま返す。
    // このパスはOpaqueブレンドでバックバッファへ描かれるため出力アルファに消費者がおらず、
    // 以前ここで行っていた事前乗算（RGBにアルファを乗算）は「HDRアルファが1未満の画素を
    // 暗くする」副作用しか持たなかった。不透明ジオメトリのアルファがHDRバッファへ漏れると
    // 透明テクセルが真っ黒に潰れる不具合の原因になっていたため取り除いている
    return float4(ldrColor, 1.0f);
}

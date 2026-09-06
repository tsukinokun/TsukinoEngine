//--------------------------------------------------------------
//! @file   Tonemap.ps.hlsl
//! @brief  トーンマッピング用ピクセルシェーダー
//! @author 山﨑愛
//--------------------------------------------------------------

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
//! @brief ACESトーンマッピング
//--------------------------------------------------------------
float3 ACES(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

//--------------------------------------------------------------
//! @brief ピクセルシェーダーメイン
//--------------------------------------------------------------
float4 PSMain(PSInput input) : SV_TARGET
{
        // UV のY を反転
    float2 uv = float2(input.uv.x, 1.0f - input.uv.y);

    // HDRバッファをサンプリング
    float4 hdrColor = hdrTexture.Sample(hdrSampler, uv);

    // ACESトーンマッピング（RGBのみ）
    float3 ldrColor = ACES(hdrColor.rgb * 0.6);

    // アルファは乗算せずそのまま返す。
    // このパスはOpaqueブレンドでバックバッファへ描かれるため出力アルファに消費者がおらず、
    // 以前ここで行っていた事前乗算（RGBにアルファを乗算）は「HDRアルファが1未満の画素を
    // 暗くする」副作用しか持たなかった。不透明ジオメトリのアルファがHDRバッファへ漏れると
    // 透明テクセルが真っ黒に潰れる不具合の原因になっていたため取り除いている
    return float4(ldrColor, 1.0f);
}

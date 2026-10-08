//--------------------------------------------------------------
//! @file   Fog.ps.hlsl
//! @brief  フォグパス用ピクセルシェーダ（距離フォグ＋高さフォグ＋ノイズ揺らぎ）
//! @author 山﨑愛
//! @note   VSはTonemap.vs.hlslを共用する（頂点バッファ不要のフルスクリーン三角形）。
//!         HDRバッファは読まず、深度(t13)だけを読んでHDRへ直接over合成する。式は Fog.hlsli にある。
//!         そのため出力はプリマルチプライ済み float4(fogColor * f, f) で、
//!         ブレンドは ONE / INV_SRC_ALPHA（DirectXTKのAlphaBlend）を前提にする。
//--------------------------------------------------------------
#pragma pack_matrix(row_major)

// シーン定数バッファ(b0)。手書きせずScene.hlsliから取り込む（理由は同ファイル参照）
#include "Scene.hlsli"
#include "Fog.hlsli"

//--------------------------------------------------------------
//! @brief 深度 (t13) と読み取り用ポイントサンプラー (s9)
//! @note  Lightingパスと同じスロットを使う（G-Bufferと共有のDSVビュー）
//--------------------------------------------------------------
Texture2D    gbufferDepth : register(t13);
SamplerState gbufferSampler : register(s9);

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
    //----------------------------------------------------------
    // フルスクリーン三角形のUVはTonemapパスと同じ生成規則。
    // NDCはそのまま、テクスチャ座標系（Y下向き）は反転して使う
    //----------------------------------------------------------
    float2 ndc      = input.uv * 2.0f - 1.0f;
    float2 screenUV = float2(input.uv.x, 1.0f - input.uv.y);

    float depth = gbufferDepth.Sample(gbufferSampler, screenUV).r;

    //----------------------------------------------------------
    // 視線方向はリバースZの最遠点（z=0）から求める（Sky.ps.hlslと同じ式）
    //----------------------------------------------------------
    float4 farPos = mul(float4(ndc, 0.0f, 1.0f), invViewProj);
    farPos /= farPos.w;

    float3 rayDir = normalize(farPos.xyz - cameraPos.xyz);

    // リバースZ：深度0 = 何も描かれていない背景（Skyパスの結果）
    bool isBackground = (depth <= 0.0f);

    float4 worldPos = mul(float4(ndc, depth, 1.0f), invViewProj);
    worldPos /= worldPos.w;

    float4 fog = ComputeFog(worldPos.xyz, rayDir, isBackground);

    //----------------------------------------------------------
    // プリマルチプライ済みで返す（ブレンドは ONE / INV_SRC_ALPHA）
    //----------------------------------------------------------
    return float4(fog.rgb * fog.a, fog.a);
}

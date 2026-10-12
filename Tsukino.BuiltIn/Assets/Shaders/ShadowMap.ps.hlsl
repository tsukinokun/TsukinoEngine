//--------------------------------------------------------------
//! @file   ShadowMap.ps.hlsl
//! @brief  シャドウマップ生成用ピクセルシェーダ
//! @author 山﨑愛
//! @note   深度はハードウェアが書くので、色は出力しない。
//!         やることは画素を抜くことだけ。
//!         - くり抜き（アルファテスト）：マテリアルの alphaCutoff が 0 より大きいとき、
//!           アルベドのアルファで形を抜く（葉のカードが四角い影を落とさないように）。
//!           GBuffer.ps.hlsl の clip と同じ判定にして、見た目と影の形を揃える
//!         - 網点フェード：DrawCommand::ditherFade（b1 の motionFlags.y）で画素を抜き、
//!           描画距離の端で物を少しずつ出すときに影も一緒に現れるようにする
//!         どちらも使わない普通の物は、分岐を素通りするだけで負荷はほぼ増えない
//--------------------------------------------------------------
#pragma pack_matrix(row_major)
#include "Material.hlsli"
#include "Dither.hlsli"

//--------------------------------------------------------------
//! @brief トランスフォーム定数バッファ (b1)。ここで読むのは網点フェード（motionFlags.y）だけ
//--------------------------------------------------------------
cbuffer CBufferTransform : register(b1)
{
    matrix world;
    matrix prevWorld;
    float4 motionFlags;    // y: 網点フェードの見えている割合（1で普通に描く）
};

Texture2D    albedoTexture : register(t0);
SamplerState albedoSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

void PSMain(PSInput input)
{
    ClipDither(input.position.xy, motionFlags.y);

    if(alphaCutoff > 0.0f)
        clip(albedoTexture.Sample(albedoSampler, input.uv).a - alphaCutoff);
}

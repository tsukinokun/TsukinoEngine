//--------------------------------------------------------------
//! @file   SphereColorGrade.ps.hlsl
//! @brief  球形の色調補正パス用ピクセルシェーダ
//! @note   VSはTonemap.vs.hlslを共用する（頂点バッファ不要のフルスクリーン三角形）。
//!         HDRバッファは読まず、深度(t13)だけを読む。出力はデュアルソースで、
//!         ブレンドは「結果 = 描かれている色 * SV_Target1 + SV_Target0」を前提にする
//!         （SphereColorGradePass が作るブレンドステート）。
//!         球の外側は (加算0, 乗算1) を返すので何も変わらない
//--------------------------------------------------------------
#pragma pack_matrix(row_major)

#include "Scene.hlsli"

//--------------------------------------------------------------
//! @brief 色調補正パラメータ (b8。フォグと相乗り)
//! @note  ConstantBuffer.hpp の CBufferSphereColorGrade と1バイト単位で一致させること
//--------------------------------------------------------------
cbuffer CBufferSphereColorGrade : register(b8)
{
    float4 gradeSphere;       // xyz: 中心, w: 半径
    float4 gradeTint;         // xyz: 乗算色, w: 全体の効き
    float4 gradeFogColor;     // xyz: 霧の色, w: 霧の濃さ
    float4 gradeEdgeColor;    // xyz: 境界の光の色, w: 光の帯の幅
    float4 gradeEdgeParams;   // x: 境界のぼかし幅
};

Texture2D    gbufferDepth : register(t13);
SamplerState gbufferSampler : register(s9);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

struct PSOutput
{
    float4 add : SV_Target0;         // 足す色
    float4 multiply : SV_Target1;    // 描かれている色へ掛ける色
};

//--------------------------------------------------------------
//! @brief ピクセルシェーダーメイン
//--------------------------------------------------------------
PSOutput PSMain(PSInput input)
{
    float2 ndc      = input.uv * 2.0f - 1.0f;
    float2 screenUV = float2(input.uv.x, 1.0f - input.uv.y);

    float depth = gbufferDepth.Sample(gbufferSampler, screenUV).r;

    // リバースZ：深度0 = 何も描かれていない背景（空）
    bool isBackground = (depth <= 0.0f);

    float4 worldPos = mul(float4(ndc, depth, 1.0f), invViewProj);
    worldPos /= worldPos.w;

    float3 center   = gradeSphere.xyz;
    float  radius   = gradeSphere.w;
    float  softness = max(gradeEdgeParams.x, 1.0e-3f);

    //----------------------------------------------------------
    // 画素から中心までの距離。空には位置が無いので、カメラが球の内側に入ったら
    // 空も内側とみなす（カメラ位置の距離で判定する）
    //----------------------------------------------------------
    float distanceToCenter = isBackground ? distance(cameraPos.xyz, center) : distance(worldPos.xyz, center);

    // 内側ほど1。境界は softness の幅でなめらかに切り替える
    float inside = 1.0f - smoothstep(radius - softness, radius, distanceToCenter);
    inside *= saturate(gradeTint.w);

    //----------------------------------------------------------
    // 内側：色を掛けてから霧の色へ寄せる。
    // 霧はカメラからの距離で濃くなる（空は最も濃い扱い）
    //----------------------------------------------------------
    float viewDistance = isBackground ? 1.0e6f : distance(worldPos.xyz, cameraPos.xyz);
    float fogAmount    = 1.0f - exp(-gradeFogColor.w * viewDistance);

    float3 insideMultiply = gradeTint.rgb * (1.0f - fogAmount);
    float3 insideAdd      = gradeFogColor.rgb * fogAmount;

    //----------------------------------------------------------
    // 境界の光：半径のすぐ外側に細い帯を足す（空には出さない）
    //----------------------------------------------------------
    float edgeWidth = max(gradeEdgeColor.w, 1.0e-3f);
    float edge      = isBackground ? 0.0f : saturate(1.0f - abs(distanceToCenter - radius) / edgeWidth);
    edge *= saturate(gradeTint.w);

    PSOutput output;
    output.multiply = float4(lerp(float3(1.0f, 1.0f, 1.0f), insideMultiply, inside), 1.0f);
    output.add      = float4(insideAdd * inside + gradeEdgeColor.rgb * edge * edge, 0.0f);
    return output;
}

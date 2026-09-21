//--------------------------------------------------------------
//! @file   Model.ps.hlsl
//! @brief  3Dモデル用のフォワードピクセルシェーダ
//! @author 山﨑愛
//! @note   不透明の通常描画はGBuffer.ps.hlsl（ディファード）が担当する。
//!         本シェーダーは半透明（Transparent）等、フォワードでの
//!         ライティングが必要な将来の用途のために残してある。
//!         BRDF・シャドウPCFの式はPBR.hlsliに一元化してあり、
//!         ディファード側（GBuffer.ps.hlsl / Lighting.ps.hlsl）と乖離しない。
//--------------------------------------------------------------
#pragma pack_matrix(row_major)
#include "PBR.hlsli"
#include "IBL.hlsli"
#include "Material.hlsli"

//--------------------------------------------------------------
//! @brief アルベドテクスチャ (t0)
//--------------------------------------------------------------
Texture2D albedoTexture : register(t0);
//--------------------------------------------------------------
//! @brief シャドウマップ (t8)
//--------------------------------------------------------------
Texture2DArray shadowMap : register(t8);    // スライス=カスケード（近→遠）
//--------------------------------------------------------------
//! @brief アルベドテクスチャ用サンプラー (s0)
//--------------------------------------------------------------
SamplerState albedoSampler : register(s0);
//--------------------------------------------------------------
//! @brief シャドウマップ用比較サンプラー (s8)
//--------------------------------------------------------------
SamplerComparisonState shadowSampler : register(s8);
//--------------------------------------------------------------
//! @brief ピクセルシェーダ入力構造体
//--------------------------------------------------------------
struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief PCFシャドウサンプリング（式はPBR.hlsliのSampleShadowPCFに一元化してある）
//! @return 遮蔽量 0.0f(暗) - 1.0f(明)
//--------------------------------------------------------------
float GetShadowPCF(float3 worldPos, float3 N, float3 L)
{
    return SampleShadowPCF(shadowMap, shadowSampler, worldPos, N, L);
}

//--------------------------------------------------------------
//! @brief ピクセルシェーダメイン関数
//--------------------------------------------------------------
float4 PSMain(PSInput input) : SV_TARGET
{
    //----------------------------------------------------------
    // マテリアルパラメータ取得
    // テクスチャがない場合はCBufferMaterialの定数値で代用
    //----------------------------------------------------------
    float4 albedoSample = albedoTexture.Sample(albedoSampler, input.uv);

    //----------------------------------------------------------
    // アルファテスト（カットアウト）
    // alphaCutoffはマテリアルのしきい値。0のときは無効化される。
    // baseColor.aを掛けないのが重要で、このシェーダーはフェード中
    // （ModelComponent::opacityがbaseColor.aへ乗算される）にも通るため、
    // 掛けてしまうとフェードが進んだ瞬間にモデル全体が消える。
    // 深度事前パス（TransparentDepth）も同じシェーダーを使うので、
    // ここで破棄しないと透明テクセルが深度を書いて背後を不正に遮蔽する
    //----------------------------------------------------------
    clip(albedoSample.a - alphaCutoff);

    // アルベドは反射率なのでトーンマップは通さない（GBuffer.ps.hlslと同じ扱い）
    float3 albedo = albedoSample.rgb * baseColor.rgb;    // テクスチャ × 定数色

    //----------------------------------------------------------
    // 法線（現状は頂点法線をそのまま使用）
    // ノーマルマップを使う場合はここでTBN変換を行う
    //----------------------------------------------------------
    float3 N = normalize(input.normal);

    //----------------------------------------------------------
    // 照明計算に必要なベクトルを算出
    //   V: 視線ベクトル（ピクセル → カメラ）
    //   L: ライトベクトル（ピクセル → ライト）
    //----------------------------------------------------------
    float3 V = normalize(cameraPos.xyz - input.worldPos);
    float3 L = normalize(-lightDir.xyz); // lightDirは「ライトが向いている方向」なので反転

    //----------------------------------------------------------
    // シャドウ係数とライト放射輝度 (radiance) を取得
    //----------------------------------------------------------
    float shadow = GetShadowPCF(input.worldPos, N, L);

    // 影を真っ黒にしない（下限はPBR.hlsliのkShadowMinLit。ディファード側と共用）
    shadow = max(shadow, kShadowMinLit);

    float3 radiance = lightColor.rgb * lightColor.w * shadow; // 色 × 強度 × 影

    // 直接照明 = Cook-Torrance BRDF × radiance × NdotL（PBR.hlsliに一元化）
    float3 directLight = EvaluatePBR(N, V, L, albedo, metallic, roughness, specular, radiance);

    //----------------------------------------------------------
    // アンビエント（スカイ由来のIBL。Lighting.ps.hlslと同じEvaluateIBLを使う）
    // このフォワードパスはAOテクスチャを持たないため ao=1.0（未遮蔽）を渡す
    //----------------------------------------------------------
    float3 ambient = EvaluateIBL(N, V, albedo, metallic, roughness, specular, 1.0f);

    //----------------------------------------------------------
    // 最終カラー合成
    //   ambient: 間接光の簡易近似
    //   directLight: 直接照明（拡散 + 鏡面）
    //   emissive: 自発光（ライティング非依存）
    //----------------------------------------------------------
    float3 finalColor = ambient + directLight + emissive;

    //----------------------------------------------------------
    // リムグロー（拾えるアイテムの強調など）＋ 面全体の一律発光の上乗せ。
    // 式はMaterial.hlsliに一元化してあり、ディファード側
    // （GBuffer.ps.hlsl）と同じ関数を呼ぶので乖離しない
    //----------------------------------------------------------
    finalColor += EvaluateEmissiveBoost(N, V);

    return float4(finalColor, baseColor.a * albedoSample.a);
}

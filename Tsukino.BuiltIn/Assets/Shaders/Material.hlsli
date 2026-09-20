//--------------------------------------------------------------
//! @file   Material.hlsli
//! @brief  マテリアル定数バッファ(b2)とリムグローの共通定義
//! @author 山﨑愛
//! @note   b2のレイアウトと、リムグローの式が乖離しないよう、ここに一元化する
//!         （PBR.hlsliが直接照明のBRDFを一元化しているのと同じ考え方）。
//!         PBR.hlsliには同居させない。Sprite.ps.hlslもb2を使うが、
//!         PBR.hlsli経由にすると不要なCBufferScene(b0)まで引き込むため。
//--------------------------------------------------------------
#ifndef TSUKINO_MATERIAL_HLSLI
#define TSUKINO_MATERIAL_HLSLI

//--------------------------------------------------------------
//! マテリアル定数バッファ (b2)
//! @note Tsukino/Renderer/ConstantBuffer.hpp の CBufferMaterial と
//!       1バイト単位で一致させること。
//!       emissivePadは詰め物だが必須。C++側のhlslpp::float3はSIMDレジスタ幅の
//!       16バイトを占めるのに対し、HLSLのfloat3は12バイトで、直後のスカラーが
//!       同じ16バイト行へ詰められてしまう。ここで1つ空けないとmetallic以降が
//!       4バイトずれ、roughnessがmetallicの値を読むといった事故になる
//--------------------------------------------------------------
cbuffer CBufferMaterial : register(b2)
{
    float4 baseColor;      // 0
    float3 emissive;       // 16
    float  emissivePad;    // 28（C++のhlslpp::float3が持つ4番目のレーンに対応）
    float  metallic;       // 32
    float  roughness;      // 36
    float  specular;       // 40
    float  alphaCutoff;    // 44（アルファテストのしきい値。0=無効）
    float4 rimColor;       // 48  xyz: ふちの色, w: ふちの強さ
    float4 rimParams;      // 64  x: ふちの鋭さ(pow指数), y: 全体の白発光量, zw: 予約
};

//--------------------------------------------------------------
//! エンティティ単位の発光上乗せ量を求めます。
//! @param  [in] N 正規化済みのワールド法線
//! @param  [in] V 正規化済みの視線ベクトル（ピクセル → カメラ）
//! @return エミッシブへ加算する発光量（リム + 全体発光）
//--------------------------------------------------------------
float3 EvaluateRimGlow(float3 N, float3 V)
{
    //----------------------------------------------------------
    // 視線に対して斜めを向いた面ほど光らせる（輪郭がネオンのように光る）。
    // 分岐は意味上のスイッチで、鋭さ0＝リム無効を表す。同時に、
    // ここから先でpowの指数が必ず正になることも保証している。
    // max()は数値ガード。powはexp2(e * log2(x))を経由するため、
    // 真正面を向いた画素（1 - NdotV が 0）でNaNになる環境があり、
    // そのままだとトーンマップ後に黒い点として出る
    //----------------------------------------------------------
    float rim = 0.0f;
    if(rimParams.x > 0.0f) {
        rim = pow(max(1.0f - saturate(dot(N, V)), 1e-6f), rimParams.x);
    }

    // 第2項は面全体を一律に持ち上げる白発光。HDRターゲットへ書くため、
    // 1.0を超えた分はトーンマップで白へ寄っていく
    return rimColor.rgb * rim * rimColor.w + rimParams.y;
}

#endif    // TSUKINO_MATERIAL_HLSLI

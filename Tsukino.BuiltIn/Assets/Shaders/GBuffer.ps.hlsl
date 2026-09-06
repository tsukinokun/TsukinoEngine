//--------------------------------------------------------------
//! @file   GBuffer.ps.hlsl
//! @brief  ディファードGBufferパス用ピクセルシェーダ
//! @author 山﨑愛
//! @note   VSはModel.vs.hlsl（スキン有り）/ ModelStatic.vs.hlsl（スキン無し）を
//!         共用する。ライティングはここでは計算せず、後段のLightingパスへ委譲する。
//--------------------------------------------------------------
#pragma pack_matrix(row_major)
#include "PBR.hlsli"

//--------------------------------------------------------------
//! @brief マテリアル定数バッファ (b2)
//--------------------------------------------------------------
cbuffer CBufferMaterial : register(b2)
{
    float4 baseColor;
    float3 emissive;
    float  metallic;
    float  roughness;
    float  specular;
    float4 rimColor;     // xyz: ふちの色, w: ふちの強さ
    float4 rimParams;    // x: ふちの鋭さ(pow指数), y: 全体の白発光量, z: alphaCutoff（0=アルファテスト無効）, w: 予約
};

//--------------------------------------------------------------
//! @brief マテリアルテクスチャ (t0〜t4)
//! @note  未設定スロットはModelSystemがデフォルト（白 / フラット法線）へ
//!        フォールバック済みなので、ここでは常に有効なテクスチャが来る前提でよい。
//!        サンプラーはSetMaterialがs0に1枚しかバインドしないため全スロットで共用する。
//--------------------------------------------------------------
Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D mrTexture : register(t2);        // G=roughness, B=metallic（glTF慣例）
Texture2D emissiveTexture : register(t3);
Texture2D aoTexture : register(t4);

SamplerState albedoSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 curClip : TEXCOORD1;     // 今フレームのクリップ座標（速度計算用）
    float4 prevClip : TEXCOORD2;    // 前フレームのクリップ座標（速度計算用）
};

//--------------------------------------------------------------
//! @brief G-Buffer出力
//--------------------------------------------------------------
struct PSOutput
{
    float4 albedo      : SV_TARGET0;    // rgb: albedo, a: 未使用
    float4 normal      : SV_TARGET1;    // rgb: ワールド法線(エンコード済み), a: 未使用（将来のShadingModel ID等に予約）
    float4 material    : SV_TARGET2;    // r: metallic, g: roughness, b: specular, a: AO
    float4 emissiveOut : SV_TARGET3;    // rgb: emissive + リム発光 + 全体白発光, a: 未使用
    float4 worldPosOut : SV_TARGET4;    // rgb: ワールド座標（頂点シェーダー補間値そのまま）, a: 未使用
    float2 velocity    : SV_TARGET5;    // rg: 1フレームあたりのUV移動量（符号付き）
};

PSOutput PSMain(PSInput input)
{
    PSOutput output;

    //----------------------------------------------------------
    // アルベド
    // Model.ps.hlsl（フォワード）と同じくACESを通す。トーンマップとの
    // 二重掛けだが、既存の見た目との差分をゼロにするため意図的に踏襲する
    // （除去は別途ポストプロセス整備のタイミングで扱う）
    //----------------------------------------------------------
    float4 albedoSample = albedoTexture.Sample(albedoSampler, input.uv);

    //----------------------------------------------------------
    // アルファテスト（カットアウト）
    // rimParams.z はマテリアルのalphaCutoff。0のときは無効化される
    // （clipは引数が0未満のときだけ破棄するため、しきい値0ならアルファ0でも残る）。
    // ここで破棄しておかないと、透明テクセルがG-Bufferと深度を書いてしまう。
    // 判定にbaseColor.aを掛けないのは、くり抜き形状はテクスチャのアルファだけが
    // 持つ情報であり、オブジェクト全体のフェード（ModelComponent::opacity）と
    // 混ぜるとフェード途中で形状が破綻するため
    //----------------------------------------------------------
    clip(albedoSample.a - rimParams.z);

    float3 albedo        = ACES(albedoSample.rgb * baseColor.rgb);

    //----------------------------------------------------------
    // 法線
    // ノーマルマップ未設定時はフラット法線(0,0,1)が来るので、
    // ApplyNormalMapを通しても頂点法線がそのまま保たれる（分岐不要）。
    //----------------------------------------------------------
    float3 vertexN      = normalize(input.normal);
    float3 tangentN     = DecodeNormal(normalTexture.Sample(albedoSampler, input.uv).rgb);
    float3 N            = ApplyNormalMap(vertexN, input.worldPos, input.uv, tangentN);

    //----------------------------------------------------------
    // メタリック・ラフネス・AO
    // テクスチャ × cbuffer定数。未設定時は白(=1.0)が来るので定数値がそのまま残る。
    // MRマップのチャンネル割り当てはglTF慣例（G=roughness, B=metallic）。
    //----------------------------------------------------------
    float4 mrSample = mrTexture.Sample(albedoSampler, input.uv);
    float  met      = metallic * mrSample.b;
    float  rough    = roughness * mrSample.g;
    float  ao       = aoTexture.Sample(albedoSampler, input.uv).r;

    //----------------------------------------------------------
    // リム発光・全体白発光（Model.ps.hlslのハイライト演出と同じ式）
    // Lightingパス側の計算を単純化するため、この段階で焼き込んでおく
    //----------------------------------------------------------
    float3 V     = normalize(cameraPos.xyz - input.worldPos);
    float  NdotV = saturate(dot(N, V)) + 1e-5f;
    float  rim   = pow(saturate(1.0f - NdotV), rimParams.x);

    float3 emissiveSample = emissiveTexture.Sample(albedoSampler, input.uv).rgb;
    float3 emissiveTotal  = emissive * emissiveSample + rimColor.rgb * rim * rimColor.w + rimParams.y;

    //----------------------------------------------------------
    // 速度（モーションブラー用）
    // クリップ座標をそれぞれ透視除算してUVへ落とし、その差分を書く。
    // 強度やシャッター補正はここでは掛けない（ブラーパス側の責務）。
    // motionFlags.x が 0 のときVSが prevClip = curClip にしているので、
    // その場合はここで自然に速度0になる。
    //----------------------------------------------------------
    float2 curUV  = (input.curClip.xy / input.curClip.w) * float2(0.5f, -0.5f) + 0.5f;
    float2 prevUV = (input.prevClip.xy / input.prevClip.w) * float2(0.5f, -0.5f) + 0.5f;

    // アルファは書かない（1.0固定）。カットアウトは上のclipで済んでおり、
    // ここでアルファを書くとLightingパス経由でHDRバッファのアルファを下げ、
    // 透明テクセルが真っ黒に潰れる原因になる
    output.albedo      = float4(albedo, 1.0f);
    output.normal      = float4(EncodeNormal(N), 0.0f);
    output.material    = float4(met, rough, specular, ao);
    output.emissiveOut = float4(emissiveTotal, 0.0f);
    output.worldPosOut = float4(input.worldPos, 0.0f);
    output.velocity    = curUV - prevUV;

    return output;
}

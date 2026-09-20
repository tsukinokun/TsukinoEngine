//--------------------------------------------------------------
//! @file   PBR.hlsli
//! @brief  Cook-Torrance PBRの共通関数とシーン定数バッファ
//! @author 山﨑愛
//! @note   フォワード(Model.ps.hlsl)とディファード(GBuffer.ps.hlsl / Lighting.ps.hlsl)で
//!         BRDFの式が乖離しないよう、ここに一元化する。
//!         トーンマップはここには無い。Tonemap.ps.hlslがHDRバッファに対して
//!         1回だけ掛けるのが唯一の場所で、アルベドやBRDFの途中結果には掛けない。
//--------------------------------------------------------------
#ifndef TSUKINO_PBR_HLSLI
#define TSUKINO_PBR_HLSLI

//--------------------------------------------------------------
//! @brief シーン用定数バッファ (b0)
//--------------------------------------------------------------
cbuffer CBufferScene : register(b0)
{
    matrix view;
    matrix projection;
    matrix viewProj;
    matrix invViewProj;
    matrix lightViewProj;    // ライト空間のViewProjection行列
    float4 lightDir;         // xyz: ライト方向（正規化済み）
    float4 lightColor;       // xyz: ライトの色, w: 強度
    float4 cameraPos;        // xyz: カメラのワールド座標, w: 未使用
    matrix prevViewProj;     // 前フレームのViewProjection行列（速度バッファ生成用）
    float4 timeParams;       // x: 起動からの経過秒, y: 前フレームからの経過秒, z: sin(x), w: cos(x)
    float4 screenParams;     // xy: 描画領域の解像度(px), zw: その逆数
    float4 shadowParams;     // x: シャドウマップの一辺(px), y: その逆数, z: 1テクセルのワールド幅, w: 1/奥行き
};

static const float PI = 3.14159265358979323846f;

//--------------------------------------------------------------
//! @brief 法線分布関数 (NDF): GGX / Trowbridge-Reitz
//! @param NdotH  法線とハーフベクトルの内積
//! @param a      粗さの2乗 (roughness^2)
//--------------------------------------------------------------
float D_GGX(float NdotH, float a)
{
    float a2    = a * a;
    float denom = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * denom * denom);
}

//--------------------------------------------------------------
//! @brief 幾何減衰関数 (GSF): Smith / Schlick-GGX
//--------------------------------------------------------------
float G_Smith(float NdotV, float NdotL, float k)
{
    float ggx1 = NdotV / (NdotV * (1.0f - k) + k);    // 視線側
    float ggx2 = NdotL / (NdotL * (1.0f - k) + k);    // ライト側
    return ggx1 * ggx2;
}

//--------------------------------------------------------------
//! @brief フレネル項: Schlick近似
//--------------------------------------------------------------
float3 F_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

//--------------------------------------------------------------
//! @brief Cook-Torrance BRDFによる1灯分の直接照明を計算する
//! @param N         法線（正規化済み）
//! @param V         視線ベクトル（ピクセル→カメラ、正規化済み）
//! @param L         ライトベクトル（ピクセル→ライト、正規化済み）
//! @param albedo    アルベド色
//! @param metallic  メタリック [0,1]
//! @param roughness ラフネス [0,1]
//! @param specular  非金属のスペキュラ強度
//! @param radiance  ライトの放射輝度（色 × 強度 × 減衰・シャドウ係数を乗算済み）
//! @return 直接照明の寄与（拡散+鏡面）
//--------------------------------------------------------------
float3 EvaluatePBR(float3 N, float3 V, float3 L, float3 albedo, float metallic, float roughness, float specular, float3 radiance)
{
    float3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V)) + 1e-5f;
    float NdotH = saturate(dot(N, H));
    float HdotV = saturate(dot(H, V));

    float rough = max(roughness, 0.15f);    // 0に近いと分母が発散するので下限を設ける
    float a     = rough * rough;
    float k     = (rough + 1.0f) * (rough + 1.0f) / 8.0f;

    // F0: 垂直入射時の反射率（誘電体はspecularから、金属はアルベドから）
    float3 F0 = lerp(0.08f * specular.xxx, albedo, metallic);

    float  D = D_GGX(NdotH, a);
    float  G = G_Smith(NdotV, NdotL, k);
    float3 F = F_Schlick(HdotV, F0);

    float3 spec = (D * G * F) / (4.0f * NdotV * NdotL + 1e-5f);

    float3 kD   = (1.0f - F) * (1.0f - metallic);    // 金属は拡散反射なし
    float3 diff = kD * albedo / PI;

    return (diff + spec) * radiance * NdotL;
}

//--------------------------------------------------------------
//! @brief ワールド法線をG-Buffer1のrgbへエンコードする ([-1,1] → [0,1])
//--------------------------------------------------------------
float3 EncodeNormal(float3 n)
{
    return n * 0.5f + 0.5f;
}

//--------------------------------------------------------------
//! @brief G-Buffer1のrgbからワールド法線を復元する
//--------------------------------------------------------------
float3 DecodeNormal(float3 enc)
{
    return normalize(enc * 2.0f - 1.0f);
}

//--------------------------------------------------------------
//! @brief リバースZの深度とスクリーンUVからワールド座標を復元する
//! @param depth       深度バッファの値（0=最遠、1=最近：リバースZ）
//! @param uv          スクリーンUV [0,1]（Y下向き、テクスチャ座標系）
//! @param invViewProjMat viewProjの逆行列
//--------------------------------------------------------------
float3 ReconstructWorldPos(float depth, float2 uv, matrix invViewProjMat)
{
    float2 ndc   = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
    float4 clip  = float4(ndc, depth, 1.0f);
    float4 world = mul(clip, invViewProjMat);
    return world.xyz / world.w;
}

//--------------------------------------------------------------
//! @brief 頂点タンジェント無しでノーマルマップを適用する
//! @param N             補間済みワールド頂点法線（正規化済み）
//! @param worldPos      ピクセルのワールド座標
//! @param uv            ピクセルのUV
//! @param tangentNormal ノーマルマップのサンプル値をデコードした接空間法線
//! @return ノーマルマップを適用したワールド法線
//! @note  画面空間微分(ddx/ddy)からTBNを組み立てる方式。
//!        頂点にTANGENT属性を足すとVertexFormat・入力レイアウト・ModelImporter・
//!        .tsmキャッシュの全再生成まで波及するため、PS内で完結するこの方式を採る。
//!        UVシームやミラーUVの境界では1ピクセル分だけ精度が落ちるが、
//!        頂点タンジェントを持たないアセットでも動作する利点が大きい。
//--------------------------------------------------------------
float3 ApplyNormalMap(float3 N, float3 worldPos, float2 uv, float3 tangentNormal)
{
    // ワールド座標とUVの画面空間微分から、UVに沿った基底を求める
    float3 dp1 = ddx(worldPos);
    float3 dp2 = ddy(worldPos);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);

    // 法線に直交する成分だけを取り出して連立を解く
    float3 dp2perp = cross(dp2, N);
    float3 dp1perp = cross(N, dp1);
    float3 T       = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B       = dp2perp * duv1.y + dp1perp * duv2.y;

    // 縮退（UVが潰れている面）でゼロ除算しないよう最大成分で正規化する。
    //
    // 【しきい値を絶対値にしてはいけない】
    // T と B には画面空間UV微分の外積 det が係数として掛かっている
    // （T = det * cross(dPdv, N)）。det は「1画素あたりのUV変化」の2乗なので、
    // 0.5mのモデルを画面いっぱいに映すと duv/px ≒ 1e-3 → det ≒ 1e-6 →
    // dot(T,T) ≒ 1e-12 まで小さくなる。
    // ここを 1e-8 のような絶対値と比べると常にそちらが勝ち、T*invmax が 0 へ
    // 潰れて mul(tangentNormal, TBN) が tn.z * N になる。
    // ＝ノーマルマップが丸ごと無効化される（画面上で概ね140px未満の
    // オブジェクトでしか効かない状態だった）。
    // 0 との比較だけにすれば、T と B の相対比のみで正規化されスケールに依らない。
    float maxLenSq = max(dot(T, T), dot(B, B));
    float invmax   = (maxLenSq > 0.0f) ? rsqrt(maxLenSq) : 0.0f;

    // maxLenSq が 0 のときは invmax も 0 になり、TBN の1行目・2行目が 0 になる。
    // その結果は ±N（＝頂点法線そのまま）で、縮退面での挙動として正しい
    float3x3 TBN = float3x3(T * invmax, B * invmax, N);
    float3   n   = mul(tangentNormal, TBN);

    // 打ち消し合って長さ0になったら normalize が NaN を返すので、頂点法線へ逃がす
    float lenSq = dot(n, n);
    return (lenSq > 1e-12f) ? n * rsqrt(lenSq) : N;
}

//--------------------------------------------------------------
// シャドウのバイアス（単位はシャドウマップ1テクセルのワールド幅＝shadowParams.z）。
//
// 一定の深度バイアスだけだと、光に対して斜めの面ほど1テクセルの中で深度が大きく
// 変わるため、自分の影を拾って縞や黒い斑点（シャドウアクネ）になる。太陽が低い
// シーンでは特にひどい。そこで次の2つを組み合わせる：
//   法線オフセット：影を引く位置を面の法線方向へ押し出す。光に対して斜めな面ほど
//                   （N・Lが小さいほど）大きく押し出す
//   傾き比例の深度バイアス：面の傾き（tan）に比例させ、真横に近い面で暴れないよう上限を付ける
// 大きくしすぎると影が足元から浮く（peter panning）ので、PCFの広がり（±1テクセル）を
// 覆える最小限にしている
//--------------------------------------------------------------
static const float kShadowNormalOffset   = 1.5f;    // 法線方向の押し出し（テクセル）。N・L=0のときの値
static const float kShadowConstantBias   = 1.0f;    // 深度バイアスの一定分（テクセル）
static const float kShadowSlopeBias      = 1.0f;    // 深度バイアスの傾き比例分（テクセル / tan）
static const float kShadowMaxSlope       = 4.0f;    // tanの上限（約76度。これより斜めな面は同じ扱い）

//--------------------------------------------------------------
//! @brief  ディレクショナルライトの影をPCF（3x3）で引く
//! @param  map      シャドウマップ（t8）
//! @param  samp     比較サンプラー（s8。リバースZなのでGREATER_EQUALで比較する）
//! @param  worldPos 影を調べる点のワールド座標
//! @param  N        その点の法線（正規化済み）
//! @param  L        光の来る方向（正規化済み）
//! @return 遮蔽量 0.0f(暗) - 1.0f(明)
//! @note   ディファード（Lighting.hlsli）とフォワード（Model.ps.hlsl）の両方がこれを使う
//--------------------------------------------------------------
float SampleShadowPCF(Texture2D map, SamplerComparisonState samp, float3 worldPos, float3 N, float3 L)
{
    const float texelWorld = shadowParams.z;
    const float NdotL      = saturate(dot(N, L));

    // 法線オフセット。光に正対する面は押し出さない
    float3 samplePos = worldPos + N * (texelWorld * kShadowNormalOffset * (1.0f - NdotL));

    float4 lightSpace = mul(float4(samplePos, 1.0f), lightViewProj);

    // クリップ座標をUV座標に変換（DirectXはY反転）
    float2 uv = lightSpace.xy * float2(0.5f, -0.5f) + 0.5f;

    // 投影範囲の外は影なし
    if(any(uv < 0.0f) || any(1.0f < uv))
        return 1.0f;

    // 傾き比例の深度バイアス。ワールド距離で求めてから深度値へ換算する（shadowParams.w = 1/奥行き）。
    // リバースZなので、光へ近づける向き＝深度値を足す向き
    const float tanTheta  = min(sqrt(saturate(1.0f - NdotL * NdotL)) / max(NdotL, 1.0e-3f), kShadowMaxSlope);
    const float biasWorld = texelWorld * (kShadowConstantBias + kShadowSlopeBias * tanTheta);
    const float depth     = lightSpace.z + biasWorld * shadowParams.w;

    float shadow = 0.0f;

    [unroll]
    for(int x = -1; x <= 1; x++) {
        [unroll]
        for(int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) * shadowParams.y;
            shadow += map.SampleCmpLevelZero(samp, uv + offset, depth);
        }
    }

    return shadow / 9.0f;
}

#endif    // TSUKINO_PBR_HLSLI

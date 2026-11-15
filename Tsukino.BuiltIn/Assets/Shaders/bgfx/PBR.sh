//--------------------------------------------------------------
//! @file   PBR.sh
//! @brief  PBR のライティングと法線の扱い
//--------------------------------------------------------------
#ifndef TSUKINO_PBR_SH
#define TSUKINO_PBR_SH

#include "Scene.sh"
#include "TextureStages.sh"

#define PI 3.14159265358979323846

// 影の中でも残す明るさ（PBR の直接光）
#define kShadowMinLit 0.25

//--------------------------------------------------------------
//! @brief  GGX の法線分布
//--------------------------------------------------------------
float D_GGX(float NdotH, float a)
{
    float a2    = a * a;
    float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

//--------------------------------------------------------------
//! @brief  Smith の幾何減衰（Schlick-GGX）
//--------------------------------------------------------------
float G_Smith(float NdotV, float NdotL, float k)
{
    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);
    return ggx1 * ggx2;
}

//--------------------------------------------------------------
//! @brief  Schlick のフレネル
//--------------------------------------------------------------
vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (vec3_splat(1.0) - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

//--------------------------------------------------------------
//! @brief  1つの光の直接光を PBR で求める
//--------------------------------------------------------------
vec3 EvaluatePBR(vec3 N, vec3 V, vec3 L, vec3 albedo, float metal, float rough0, float spec, vec3 radiance)
{
    vec3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V)) + 1e-5;
    float NdotH = saturate(dot(N, H));
    float HdotV = saturate(dot(H, V));

    float rough = max(rough0, 0.15);    // 0に近いと分母が発散するので下限を設ける
    float a     = rough * rough;
    float k     = (rough + 1.0) * (rough + 1.0) / 8.0;

    vec3 F0 = mix(vec3_splat(0.08 * spec), albedo, vec3_splat(metal));

    float D = D_GGX(NdotH, a);
    float G = G_Smith(NdotV, NdotL, k);
    vec3  F = F_Schlick(HdotV, F0);

    vec3 specTerm = (D * G * F) / (4.0 * NdotV * NdotL + 1e-5);
    vec3 kD       = (vec3_splat(1.0) - F) * (1.0 - metal);
    vec3 diff     = kD * albedo / PI;

    return (diff + specTerm) * radiance * NdotL;
}

//--------------------------------------------------------------
//! @brief  法線を 0〜1 へ詰める / 戻す
//--------------------------------------------------------------
vec3 EncodeNormal(vec3 n)
{
    return n * 0.5 + 0.5;
}

vec3 DecodeNormal(vec3 enc)
{
    return normalize(enc * 2.0 - 1.0);
}

//--------------------------------------------------------------
//! @brief  法線マップを、接線を持たないメッシュに画面の微分で当てる
//--------------------------------------------------------------
vec3 ApplyNormalMap(vec3 N, vec3 worldPos, vec2 uv, vec3 tangentNormal)
{
    // 画面の下向きを +y にする（bgfx の dFdy は Direct3D の ddy と符号が逆）
    vec3 dp1  = dFdx(worldPos);
    vec3 dp2  = -dFdy(worldPos);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = -dFdy(uv);

    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T       = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B       = dp2perp * duv1.y + dp1perp * duv2.y;

    float maxLenSq = max(dot(T, T), dot(B, B));
    float invmax   = (maxLenSq > 0.0) ? inversesqrt(maxLenSq) : 0.0;

    // 接空間の法線を、行 T・B・N の重ね合わせで戻す
    vec3 n = tangentNormal.x * (T * invmax) + tangentNormal.y * (B * invmax) + tangentNormal.z * N;

    float lenSq = dot(n, n);
    return (lenSq > 1e-12) ? n * inversesqrt(lenSq) : N;
}

//--------------------------------------------------------------
// 影（カスケードシャドウマップ）。
// シャドウマップは配列のテクスチャ（1枚 = カスケード1つ。近い順）で、比較サンプラーで読む
// （リバース Z なので GREATER_EQUAL。C++ 側の ShadowPass がテクスチャに付けている）
//--------------------------------------------------------------
SAMPLER2DARRAYSHADOW(s_shadowMap, STAGE_SHADOW_MAP);

#define kShadowNormalOffset 1.5    // 法線方向の押し出し（テクセル）。N・L=0 のときの値
#define kShadowConstantBias 1.0    // 深度バイアスの一定分（テクセル）
#define kShadowSlopeBias    2.5    // 深度バイアスの傾き比例分（テクセル / tan）
#define kShadowMaxSlope     4.0    // tan の上限（約76度。これより斜めな面は同じ扱い）

// カスケードの境目で隣のカスケードと混ぜる幅（受け持つ奥行きの幅に対する割合）
#define kCascadeBlendBand 0.1

//--------------------------------------------------------------
//! @brief  点のカメラのビュー空間の奥行き（カスケードの分割距離と比べる値）
//--------------------------------------------------------------
float ShadowViewDepth(vec3 worldPos)
{
    return mul(vec4(worldPos, 1.0), view).z;
}

//--------------------------------------------------------------
//! @brief  画素が使う影のカスケードを選ぶ
//! @param  viewDepth [in] 画素のビュー空間の奥行き
//! @return カスケード番号（0 が最も近い）。TSUKINO_SHADOW_CASCADE_COUNT なら影を出す奥行きの外
//! @note   奥行きより手前で終わっているカスケードの数が、そのまま選ぶ番号になる
//--------------------------------------------------------------
int SelectShadowCascade(float viewDepth)
{
    int cascade = 0;
    for(int split = 0; split < TSUKINO_SHADOW_CASCADE_COUNT; ++split)
        cascade += (cascadeSplitFar[split] < viewDepth) ? 1 : 0;

    return cascade;
}

//--------------------------------------------------------------
//! @brief  1枚のカスケードから影を PCF（3x3）で引く
//! @param  worldPos [in]  影を調べる点のワールド座標
//! @param  N        [in]  その点の法線（正規化済み）
//! @param  NdotL    [in]  saturate(dot(N, L))
//! @param  tanTheta [in]  光に対する面の傾き（tan。上限付き）
//! @param  cascade  [in]  引くカスケード
//! @param  shadow   [out] 遮られていない割合 0.0(影) - 1.0(明)
//! @return true: 引けた, false: 点がこのカスケードの範囲の外
//--------------------------------------------------------------
bool SampleCascadePCF(vec3 worldPos, vec3 N, float NdotL, float tanTheta, int cascade, out float shadow)
{
    shadow = 1.0;

    float texelWorld = cascadeTexelWorld[cascade];

    // 法線オフセット。光に正対する面は押し出さない
    vec3 samplePos = worldPos + N * (texelWorld * kShadowNormalOffset * (1.0 - NdotL));

    vec4 lightSpace = mul(vec4(samplePos, 1.0), cascadeViewProj(cascade));

    // クリップ座標を UV へ（Y は反転）。平行投影なので w は常に 1
    vec2 uv = lightSpace.xy * vec2(0.5, -0.5) + 0.5;

    if(uv.x < 0.0 || uv.y < 0.0 || 1.0 < uv.x || 1.0 < uv.y)
        return false;

    // 傾き比例の深度バイアス。ワールド距離で求めてから深度値へ換算する（shadowParams.w = 1/奥行き）。
    // リバース Z なので、光へ近づける向き＝深度値を足す向き
    float biasWorld = texelWorld * (kShadowConstantBias + kShadowSlopeBias * tanTheta);
    float depth     = lightSpace.z + biasWorld * shadowParams.w;

    // 描画品質を下げているときは1タップで済ませる（比較サンプラーの線形補間だけは効く）
    if(qualityParams.x < 0.5) {
        shadow = shadow2DArray(s_shadowMap, vec4(uv, float(cascade), depth));
        return true;
    }

    float sum = 0.0;
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(float(x), float(y)) * shadowParams.y;
            sum += shadow2DArray(s_shadowMap, vec4(uv + offset, float(cascade), depth));
        }
    }

    shadow = sum / 9.0;
    return true;
}

//--------------------------------------------------------------
//! @brief  太陽の影をカスケードシャドウマップから引く
//! @param  worldPos [in] 影を調べる点のワールド座標
//! @param  N        [in] その点の法線（正規化済み）
//! @param  L        [in] 光の来る方向（正規化済み）
//! @return 遮られていない割合 0.0(影) - 1.0(明)
//! @note   ディファード（Lighting.sh）とフォワード（ForwardModel.sh）の両方がこれを使う
//--------------------------------------------------------------
float SampleShadowPCF(vec3 worldPos, vec3 N, vec3 L)
{
    float NdotL = saturate(dot(N, L));

    // 光と平行に近い面で tan が発散するので上限を付ける
    float tanTheta = min(sqrt(saturate(1.0 - NdotL * NdotL)) / max(NdotL, 1.0e-3), kShadowMaxSlope);

    float viewDepth = ShadowViewDepth(worldPos);

    // 法線オフセットで押し出した点や、別のカメラで描く UI の層のモデルでは、選んだカスケードから
    // 外れることがある。そのときだけ1つ外側へ回す（ほとんどの画素は1周目で抜ける）
    for(int cascade = SelectShadowCascade(viewDepth); cascade < TSUKINO_SHADOW_CASCADE_COUNT; ++cascade) {
        float shadow;
        if(!SampleCascadePCF(worldPos, N, NdotL, tanTheta, cascade, shadow))
            continue;

        // 境目の手前では1つ外側のカスケードと混ぜて、影の細かさが段にならないようにする。
        // 最後のカスケードは外側が無いので、明るい側（影なし）へ消していく
        float splitFar  = cascadeSplitFar[cascade];
        float splitNear = (cascade == 0) ? 0.0 : cascadeSplitFar[max(cascade, 1) - 1];
        float band      = max((splitFar - splitNear) * kCascadeBlendBand, 1.0e-4);
        float t         = saturate((viewDepth - (splitFar - band)) / band);

        if(t > 0.0) {
            float next = 1.0;
            if(cascade + 1 < TSUKINO_SHADOW_CASCADE_COUNT) {
                if(!SampleCascadePCF(worldPos, N, NdotL, tanTheta, cascade + 1, next))
                    next = shadow;    // 外側でも範囲外なら混ぜない
            }

            shadow = mix(shadow, next, t);
        }

        return shadow;
    }

    // どのカスケードにも入らなかった（＝影を出す奥行きの外）
    return 1.0;
}

#endif    // TSUKINO_PBR_SH

//--------------------------------------------------------------
//! @file   Fog.ps.hlsl
//! @brief  フォグパス用ピクセルシェーダ（距離フォグ＋高さフォグ＋ノイズ揺らぎ）
//! @author 山﨑愛
//! @note   VSはTonemap.vs.hlslを共用する（頂点バッファ不要のフルスクリーン三角形）。
//!         HDRバッファは読まず、深度(t13)だけを読んでHDRへ直接over合成する。
//!         そのため出力はプリマルチプライ済み float4(fogColor * f, f) で、
//!         ブレンドは ONE / INV_SRC_ALPHA（DirectXTKのAlphaBlend）を前提にする。
//--------------------------------------------------------------
#pragma pack_matrix(row_major)

//--------------------------------------------------------------
//! @brief シーン定数バッファ (b0)
//--------------------------------------------------------------
cbuffer CBufferScene : register(b0)
{
    matrix view;
    matrix projection;
    matrix viewProj;
    matrix invViewProj;
    matrix lightViewProj;
    float4 lightDir;
    float4 lightColor;
    float4 cameraPos;
};

//--------------------------------------------------------------
//! @brief フォグ定数バッファ (b9)
//! @note  ConstantBuffer.hpp の CBufferFog と1バイト単位で一致させること
//--------------------------------------------------------------
cbuffer CBufferFog : register(b9)
{
    float4 fogColor;          // xyz: フォグ色(linear), w: 距離フォグ密度
    float4 distanceParams;    // x: 開始距離, y: 最大不透明度, z: 高さフォグ有効(0/1), w: 予約
    float4 heightParams;      // x: 基準高さ, y: 高さ減衰, z: 高さフォグ密度, w: 予約
    float4 sunFogColor;       // xyz: 太陽方向の散乱色, w: 散乱の鋭さ(pow指数)
    float4 noiseParams;       // x: ノイズスケール, y: ノイズ強度, z: 経過時間, w: ノイズ有効(0/1)
    float4 windParams;        // xyz: 風向き(正規化済み), w: 風速
};

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
//! @brief 背景ピクセルの距離（無限遠の代わりに使う十分大きな値）
//! @note  背景には距離フォグを掛けず、高さフォグも無限遠積分を使うため、
//!        この値が効くのはノイズのサンプル区間を決めるときだけ。
//!        そこでも FOG_NOISE_SAMPLE_MAX_CELLS で頭打ちになる。
//--------------------------------------------------------------
static const float FOG_SKY_RAY_LENGTH = 1.0e6f;

//--------------------------------------------------------------
//! @brief ノイズをサンプルする距離の上限（ノイズセル数）
//! @note  セル数で持つことで、noiseScaleを変えるだけで
//!        どんなワールドスケールでも同じ見た目になる
//--------------------------------------------------------------
static const float FOG_NOISE_SAMPLE_MAX_CELLS = 8.0f;

//--------------------------------------------------------------
//! @brief 視線に沿ってノイズを取るサンプル数
//! @note  1点だけだとレイ全体が同じ倍率になり「霧の塊が手前にある」
//!        感じが出ない。等間隔で数点取って平均すると、局所的に濃い
//!        塊がその奥のものだけを隠すようになる。
//!        コストはこの数に比例するので上げすぎないこと。
//--------------------------------------------------------------
#define FOG_NOISE_TAPS 3

//--------------------------------------------------------------
//! @brief 3D座標から0.0〜1.0の擬似乱数を返すハッシュ
//--------------------------------------------------------------
float Hash31(float3 p)
{
    p = frac(p * 0.3183099f + float3(0.71f, 0.113f, 0.419f));
    p *= 17.0f;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

//--------------------------------------------------------------
//! @brief 3Dバリューノイズ（8隅のハッシュを三線形補間）
//--------------------------------------------------------------
float ValueNoise3D(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);

    // エルミート補間で格子の境目を滑らかにする
    f = f * f * (3.0f - 2.0f * f);

    float n000 = Hash31(i + float3(0.0f, 0.0f, 0.0f));
    float n100 = Hash31(i + float3(1.0f, 0.0f, 0.0f));
    float n010 = Hash31(i + float3(0.0f, 1.0f, 0.0f));
    float n110 = Hash31(i + float3(1.0f, 1.0f, 0.0f));
    float n001 = Hash31(i + float3(0.0f, 0.0f, 1.0f));
    float n101 = Hash31(i + float3(1.0f, 0.0f, 1.0f));
    float n011 = Hash31(i + float3(0.0f, 1.0f, 1.0f));
    float n111 = Hash31(i + float3(1.0f, 1.0f, 1.0f));

    float nx00 = lerp(n000, n100, f.x);
    float nx10 = lerp(n010, n110, f.x);
    float nx01 = lerp(n001, n101, f.x);
    float nx11 = lerp(n011, n111, f.x);

    float nxy0 = lerp(nx00, nx10, f.y);
    float nxy1 = lerp(nx01, nx11, f.y);

    return lerp(nxy0, nxy1, f.z);
}

//--------------------------------------------------------------
//! @brief 2オクターブのfbm（濃淡に大小の粒を混ぜる）
//--------------------------------------------------------------
float Fbm(float3 p)
{
    return ValueNoise3D(p) * 0.65f + ValueNoise3D(p * 2.03f) * 0.35f;
}

//--------------------------------------------------------------
//! @brief 視点の高さでの霧の密度
//! @note  density(y) = heightDensity * exp(-(y - height) * falloff)
//!        カメラが基準高さより大きく下にいると exp() が発散するので指数をクランプする。
//--------------------------------------------------------------
float FogDensityAtEye(float camY)
{
    float exponent = clamp(-(camY - heightParams.x) * max(heightParams.y, 1.0e-4f), -32.0f, 32.0f);
    return heightParams.z * exp(exponent);
}

//--------------------------------------------------------------
//! @brief 高さ方向に指数減衰する密度を、視線に沿って有限の長さだけ積分する
//! @param camY   [in] カメラの高さ
//! @param dirY   [in] 視線方向のY成分
//! @param rayLen [in] 積分する長さ
//! @return 光学的厚さ（0以上）
//! @note  ジオメトリに当たったピクセル用。奥行きが有限なので発散しない。
//!        dirY * falloff が0に近いと 1/k の項が発散するため、
//!        その極限値である rayLen へフォールバックする。
//--------------------------------------------------------------
float IntegrateHeightFog(float camY, float dirY, float rayLen)
{
    float falloff      = max(heightParams.y, 1.0e-4f);
    float densityAtEye = FogDensityAtEye(camY);

    float k = dirY * falloff;

    // k がほぼ0（水平方向）なら密度は一定とみなせるので単純な掛け算になる
    if(abs(k) < 1.0e-4f)
        return min(densityAtEye * rayLen, 32.0f);

    float integral = densityAtEye * (1.0f - exp(clamp(-rayLen * k, -32.0f, 32.0f))) / k;

    return clamp(integral, 0.0f, 32.0f);
}

//--------------------------------------------------------------
//! @brief 高さ方向に指数減衰する密度を、視線に沿って無限遠まで積分する
//! @param camY    [in] カメラの高さ
//! @param dirYAbs [in] 視線方向のY成分の絶対値
//! @return 光学的厚さ（0以上）
//! @note  ジオメトリが無い背景ピクセル用。
//!        上向き（dirY > 0）なら密度が指数的に薄くなるので densityAtEye / k に収束する。
//!        下向きはこの密度モデルだと下へ行くほど濃くなり続けるため必ず発散する。
//!        そこで |dirY| で折り返し、地平線を挟んで対称に霞ませる。
//!        「地平線が一番濃く、上を向いても下を向いても薄くなる」という
//!        素直な絵になり、Skyパスが描いた地面色を塗り潰さずに済む。
//--------------------------------------------------------------
float IntegrateHeightFogToInfinity(float camY, float dirYAbs)
{
    float falloff      = max(heightParams.y, 1.0e-4f);
    float densityAtEye = FogDensityAtEye(camY);

    float k = max(dirYAbs, 1.0e-4f) * falloff;

    return min(densityAtEye / k, 32.0f);
}

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

    //----------------------------------------------------------
    // リバースZ：深度0 = 何も描かれていない背景（Skyパスの結果）
    //
    // 背景には距離フォグを掛けない。掛けると天頂までフォグ色で一様に
    // 潰れてしまうため、高さフォグの無限遠積分だけを適用する。
    // これで地平線が一番濃く、上を向くほど素の空が見える。
    //
    // 地面メッシュを置かず、Skyパスが描いた地面色が
    // 画面下半分を占める場合も、そこは背景として扱われる。
    //----------------------------------------------------------
    bool  isBackground = (depth <= 0.0f);
    float dist         = FOG_SKY_RAY_LENGTH;

    if(!isBackground) {
        float4 worldPos = mul(float4(ndc, depth, 1.0f), invViewProj);
        worldPos /= worldPos.w;

        dist = length(worldPos.xyz - cameraPos.xyz);
    }

    //----------------------------------------------------------
    // 距離フォグ（開始距離より手前は素通し）
    //----------------------------------------------------------
    float distOptical = 0.0f;
    if(!isBackground)
        distOptical = fogColor.w * max(0.0f, dist - distanceParams.x);

    //----------------------------------------------------------
    // 高さフォグ
    //----------------------------------------------------------
    float heightOptical = 0.0f;
    if(distanceParams.z > 0.5f) {
        heightOptical = isBackground ? IntegrateHeightFogToInfinity(cameraPos.y, abs(rayDir.y))
                                     : IntegrateHeightFog(cameraPos.y, rayDir.y, dist);
    }

    float optical = distOptical + heightOptical;

    //----------------------------------------------------------
    // ノイズで密度を変調して霧をゆっくり流す
    //
    // フォグ区間を等間隔にサンプルして平均を取る。1点だけだと
    // レイ全体が同じ倍率になり、濃淡が「画面に貼り付いた模様」に
    // 見えてしまう。
    //----------------------------------------------------------
    if(noiseParams.w > 0.5f) {
        // サンプルする区間の長さをノイズセル数から決める
        float maxSampleDist = FOG_NOISE_SAMPLE_MAX_CELLS / max(noiseParams.x, 1.0e-6f);
        float segmentLength = min(dist, maxSampleDist);

        // windSpeedはワールド空間の速さなので、座標と同じくスケールしてから足す
        float3 windOffset = windParams.xyz * (windParams.w * noiseParams.z * noiseParams.x);

        float sum = 0.0f;

        [unroll]
        for(int i = 0; i < FOG_NOISE_TAPS; ++i) {
            float  t         = (float(i) + 0.5f) / float(FOG_NOISE_TAPS);
            float3 samplePos = cameraPos.xyz + rayDir * (segmentLength * t);

            sum += Fbm(samplePos * noiseParams.x + windOffset);
        }

        float n = sum / float(FOG_NOISE_TAPS);

        optical *= lerp(1.0f - noiseParams.y, 1.0f + noiseParams.y, n);
    }

    //----------------------------------------------------------
    // 光学的厚さ → 不透明度
    //----------------------------------------------------------
    float fogFactor = 1.0f - exp(-clamp(optical, 0.0f, 32.0f));
    fogFactor       = min(fogFactor, distanceParams.y);

    //----------------------------------------------------------
    // 太陽方向を覗き込むほどフォグ色を散乱色へ寄せる
    // （lightDirは「ライトが向いている方向」なので反転して太陽方向にする）
    //----------------------------------------------------------
    float  sunAmount = saturate(dot(rayDir, normalize(-lightDir.xyz)));
    float3 color     = lerp(fogColor.rgb, sunFogColor.rgb, pow(sunAmount, max(sunFogColor.w, 1.0e-2f)));

    //----------------------------------------------------------
    // プリマルチプライ済みで返す（ブレンドは ONE / INV_SRC_ALPHA）
    //----------------------------------------------------------
    return float4(color * fogFactor, fogFactor);
}

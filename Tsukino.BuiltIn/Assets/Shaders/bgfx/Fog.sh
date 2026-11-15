//--------------------------------------------------------------
//! @file   Fog.sh
//! @brief  フォグの式（Fog.hlsli の bgfx 版。距離フォグ＋高さフォグ＋ノイズ揺らぎ）
//! @note   Fog.ps.sc（不透明の絵へ重ねるフォグパス）・ForwardModel.sh（半透明モデル）・
//!         Sprite.ps.sc（ワールド空間のスプライト）で共用する。
//!         シーン定数を使うので、Scene.sh を先に include しておくこと。
//!         定数（DX11 版の b8。C++ の CBufferFog）は vec4 の配列 u_fog。
//!         distanceParams.w は「モデルのシェーダーの中でフォグを掛けるか」。フォグパスでは 0、
//!         半透明のパスの描画では 1、それ以外の描画では全体が 0 で届く（FogPass 参照）
//--------------------------------------------------------------
#ifndef TSUKINO_FOG_SH
#define TSUKINO_FOG_SH

#define TSUKINO_FOG_VEC4_COUNT 7    // sizeof(CBufferFog) / 16

uniform vec4 u_fog[TSUKINO_FOG_VEC4_COUNT];

#define fogColor       u_fog[0]    // xyz: フォグ色(linear), w: 距離フォグ密度
#define distanceParams u_fog[1]    // x: 開始距離, y: 最大不透明度, z: 高さフォグ有効(0/1), w: フォワードで掛ける(0/1)
#define heightParams   u_fog[2]    // x: 基準高さ, y: 高さ減衰, z: 高さフォグ密度
#define sunFogColor    u_fog[3]    // xyz: 太陽方向の散乱色, w: 散乱の鋭さ(pow指数)
#define noiseParams    u_fog[4]    // x: ノイズスケール, y: ノイズ強度, z: 経過時間, w: ノイズ有効(0/1)
#define windParams     u_fog[5]    // xyz: 風向き(正規化済み), w: 風速
#define originParams   u_fog[6]    // xyz: 距離フォグの基準点, w: 有効(0/1)。0ならカメラ位置を使う

// 背景ピクセルの距離（無限遠の代わりに使う十分大きな値）
#define FOG_SKY_RAY_LENGTH 1.0e6

// ノイズをサンプルする距離の上限（ノイズセル数）
#define FOG_NOISE_SAMPLE_MAX_CELLS 8.0

// 視線に沿ってノイズを取るサンプル数
#define FOG_NOISE_TAPS 3

//--------------------------------------------------------------
//! @brief  3D座標から 0.0〜1.0 の擬似乱数を返すハッシュ
//--------------------------------------------------------------
float Hash31(vec3 pIn)
{
    vec3 p = fract(pIn * 0.3183099 + vec3(0.71, 0.113, 0.419));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

//--------------------------------------------------------------
//! @brief  3Dバリューノイズ（8隅のハッシュを三線形補間）
//--------------------------------------------------------------
float ValueNoise3D(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);

    // エルミート補間で格子の境目を滑らかにする
    f = f * f * (3.0 - 2.0 * f);

    float n000 = Hash31(i + vec3(0.0, 0.0, 0.0));
    float n100 = Hash31(i + vec3(1.0, 0.0, 0.0));
    float n010 = Hash31(i + vec3(0.0, 1.0, 0.0));
    float n110 = Hash31(i + vec3(1.0, 1.0, 0.0));
    float n001 = Hash31(i + vec3(0.0, 0.0, 1.0));
    float n101 = Hash31(i + vec3(1.0, 0.0, 1.0));
    float n011 = Hash31(i + vec3(0.0, 1.0, 1.0));
    float n111 = Hash31(i + vec3(1.0, 1.0, 1.0));

    float nx00 = mix(n000, n100, f.x);
    float nx10 = mix(n010, n110, f.x);
    float nx01 = mix(n001, n101, f.x);
    float nx11 = mix(n011, n111, f.x);

    float nxy0 = mix(nx00, nx10, f.y);
    float nxy1 = mix(nx01, nx11, f.y);

    return mix(nxy0, nxy1, f.z);
}

//--------------------------------------------------------------
//! @brief  2オクターブの fbm（濃淡に大小の粒を混ぜる）
//--------------------------------------------------------------
float Fbm(vec3 p)
{
    return ValueNoise3D(p) * 0.65 + ValueNoise3D(p * 2.03) * 0.35;
}

//--------------------------------------------------------------
//! @brief  視点の高さでの霧の密度
//! @note   density(y) = heightDensity * exp(-(y - height) * falloff)
//!         カメラが基準高さより大きく下にいると exp() が発散するので指数をクランプする
//--------------------------------------------------------------
float FogDensityAtEye(float camY)
{
    float exponent = clamp(-(camY - heightParams.x) * max(heightParams.y, 1.0e-4), -32.0, 32.0);
    return heightParams.z * exp(exponent);
}

//--------------------------------------------------------------
//! @brief  高さ方向に指数減衰する密度を、視線に沿って有限の長さだけ積分する
//! @param  camY   [in] カメラの高さ
//! @param  dirY   [in] 視線方向の Y 成分
//! @param  rayLen [in] 積分する長さ
//! @return 光学的厚さ（0以上）
//--------------------------------------------------------------
float IntegrateHeightFog(float camY, float dirY, float rayLen)
{
    float falloff      = max(heightParams.y, 1.0e-4);
    float densityAtEye = FogDensityAtEye(camY);

    float k = dirY * falloff;

    // k がほぼ0（水平方向）なら密度は一定とみなせるので単純な掛け算になる
    if(abs(k) < 1.0e-4)
        return min(densityAtEye * rayLen, 32.0);

    float integral = densityAtEye * (1.0 - exp(clamp(-rayLen * k, -32.0, 32.0))) / k;

    return clamp(integral, 0.0, 32.0);
}

//--------------------------------------------------------------
//! @brief  高さ方向に指数減衰する密度を、視線に沿って無限遠まで積分する
//! @param  camY    [in] カメラの高さ
//! @param  dirYAbs [in] 視線方向の Y 成分の絶対値
//! @return 光学的厚さ（0以上）
//! @note   ジオメトリが無い背景ピクセル用。|dirY| で折り返し、地平線を挟んで対称に霞ませる
//--------------------------------------------------------------
float IntegrateHeightFogToInfinity(float camY, float dirYAbs)
{
    float falloff      = max(heightParams.y, 1.0e-4);
    float densityAtEye = FogDensityAtEye(camY);

    float k = max(dirYAbs, 1.0e-4) * falloff;

    return min(densityAtEye / k, 32.0);
}

//--------------------------------------------------------------
//! @brief  1画素ぶんのフォグを求める
//! @param  worldPos     [in] 画素のワールド座標（isBackground のときは使わない）
//! @param  rayDir       [in] カメラから画素への向き（正規化済み）
//! @param  isBackground [in] 何も描かれていない背景（空）か
//! @return xyz: フォグの色, w: フォグの不透明度（0〜distanceParams.y）
//--------------------------------------------------------------
vec4 ComputeFog(vec3 worldPos, vec3 rayDir, bool isBackground)
{
    float dist = FOG_SKY_RAY_LENGTH;

    //----------------------------------------------------------
    // 距離フォグ（開始距離より手前は素通し）。背景には掛けない
    // （掛けると天頂までフォグ色で一様に潰れるので、高さフォグの無限遠積分だけを使う）。
    // 基準点は既定でカメラ位置だが、originParams.w > 0.5 なら originParams.xyz を使う
    //----------------------------------------------------------
    float distOptical = 0.0;

    if(!isBackground) {
        dist = length(worldPos - cameraPos.xyz);

        vec3  fogOrigin = (originParams.w > 0.5) ? originParams.xyz : cameraPos.xyz;
        float fogDist   = length(worldPos - fogOrigin);
        distOptical     = fogColor.w * max(0.0, fogDist - distanceParams.x);
    }

    //----------------------------------------------------------
    // 高さフォグ
    //----------------------------------------------------------
    float heightOptical = 0.0;
    if(distanceParams.z > 0.5) {
        if(isBackground)
            heightOptical = IntegrateHeightFogToInfinity(cameraPos.y, abs(rayDir.y));
        else
            heightOptical = IntegrateHeightFog(cameraPos.y, rayDir.y, dist);
    }

    float optical = distOptical + heightOptical;

    //----------------------------------------------------------
    // ノイズで密度を変調して霧をゆっくり流す。
    // フォグ区間を等間隔にサンプルして平均を取る（1点だけだと濃淡が画面に貼り付いた模様に見える）
    //----------------------------------------------------------
    if(noiseParams.w > 0.5) {
        // サンプルする区間の長さをノイズセル数から決める
        float maxSampleDist = FOG_NOISE_SAMPLE_MAX_CELLS / max(noiseParams.x, 1.0e-6);
        float segmentLength = min(dist, maxSampleDist);

        // windSpeed はワールド空間の速さなので、座標と同じくスケールしてから足す
        vec3 windOffset = windParams.xyz * (windParams.w * noiseParams.z * noiseParams.x);

        float sum = 0.0;
        for(int i = 0; i < FOG_NOISE_TAPS; ++i) {
            float t         = (float(i) + 0.5) / float(FOG_NOISE_TAPS);
            vec3  samplePos = cameraPos.xyz + rayDir * (segmentLength * t);

            sum += Fbm(samplePos * noiseParams.x + windOffset);
        }

        float n = sum / float(FOG_NOISE_TAPS);

        optical *= mix(1.0 - noiseParams.y, 1.0 + noiseParams.y, n);
    }

    //----------------------------------------------------------
    // 光学的厚さ → 不透明度
    //----------------------------------------------------------
    float fogFactor = 1.0 - exp(-clamp(optical, 0.0, 32.0));
    fogFactor       = min(fogFactor, distanceParams.y);

    //----------------------------------------------------------
    // 太陽方向を覗き込むほどフォグ色を散乱色へ寄せる
    // （lightDir は「ライトが向いている方向」なので反転して太陽方向にする）
    //----------------------------------------------------------
    float sunAmount = saturate(dot(rayDir, normalize(-lightDir.xyz)));
    vec3  color     = mix(fogColor.rgb, sunFogColor.rgb, pow(sunAmount, max(sunFogColor.w, 1.0e-2)));

    return vec4(color, fogFactor);
}

#endif    // TSUKINO_FOG_SH

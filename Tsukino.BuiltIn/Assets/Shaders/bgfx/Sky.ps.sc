$input v_texcoord0

//--------------------------------------------------------------
//! @file   Sky.ps.sc
//! @brief  スカイ（大気散乱）のピクセルシェーダー
//! @note   大気散乱の定数（C++ の CBufferSky）は vec4 の配列 u_sky。
//!         Scene.sh と同じく、メンバーは #define で配列の要素へ当てる
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"

#define SKY_PI 3.14159265358979

#define TSUKINO_SKY_VEC4_COUNT 4    // sizeof(CBufferSky) / 16

uniform vec4 u_sky[TSUKINO_SKY_VEC4_COUNT];

#define rayleighScattering u_sky[0].x
#define mieScattering      u_sky[0].y
#define mieAnisotropy      u_sky[0].z
#define sunIntensity       u_sky[0].w
#define atmosphereHeight   u_sky[1].x
#define planetRadius       u_sky[1].y
#define sunDiskSize        u_sky[1].z
#define groundColor        u_sky[2]
#define sunDirection       u_sky[3]    // xyz: 太陽方向（ライトの逆方向）

//--------------------------------------------------------------
//! @brief  レイリー位相関数
//--------------------------------------------------------------
float RayleighPhase(float cosTheta)
{
    return (3.0 / (16.0 * SKY_PI)) * (1.0 + cosTheta * cosTheta);
}

//--------------------------------------------------------------
//! @brief  ミー位相関数（Henyey-Greenstein）
//--------------------------------------------------------------
float MiePhase(float cosTheta, float g)
{
    float g2    = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosTheta;
    return (1.0 / (4.0 * SKY_PI)) * ((1.0 - g2) / pow(abs(denom), 1.5));
}

void main()
{
    //----------------------------------------------------------
    // NDC 座標からワールド空間の視線方向を逆算
    //----------------------------------------------------------
    vec2 ndc = v_texcoord0 * 2.0 - 1.0;

    // リバース Z の最遠点（z = 0）をワールド座標に変換
    vec4 farPos = mul(vec4(ndc, 0.0, 1.0), invViewProj);
    farPos /= farPos.w;

    vec3 rayDir = normalize(farPos.xyz - cameraPos.xyz);

    //----------------------------------------------------------
    // 大気散乱の計算（シーン原点を地表とするローカルスケール）
    //----------------------------------------------------------
    vec3 sunDir = normalize(sunDirection.xyz);

    float cosTheta = dot(rayDir, sunDir);

    // 上方向ほど大気を通る距離が短い、地平線方向ほど長い
    float cosAngle  = max(rayDir.y, 0.001);    // ゼロ除算防止
    float rayLength = min(atmosphereHeight / cosAngle, atmosphereHeight * 50.0);

    //----------------------------------------------------------
    // レイマーチング（サンプル数16）
    //----------------------------------------------------------
    float stepSize = rayLength / 16.0;

    vec3  rayleighCoeff = vec3(5.8e-6, 13.5e-6, 33.1e-6) * rayleighScattering;
    float mieCoeff      = 2.0e-6 * mieScattering;

    vec3 rayleighAccum = vec3_splat(0.0);
    vec3 mieAccum      = vec3_splat(0.0);
    vec3 transmittance = vec3_splat(1.0);

    for(int i = 0; i < 16; ++i) {
        // サンプル位置の高度（シーン原点=地表からの高さ）
        float sampleHeight = (float(i) + 0.5) * stepSize * cosAngle;
        float heightRatio  = saturate(sampleHeight / atmosphereHeight);

        float rayleighDensity = exp(-heightRatio * 8.0);
        float mieDensity      = exp(-heightRatio * 1.2);

        vec3 rayleighExtinction = rayleighCoeff * rayleighDensity;
        vec3 mieExtinction      = vec3_splat(mieCoeff * mieDensity * 1.1);

        vec3 extinction = rayleighExtinction + mieExtinction;

        transmittance *= exp(-extinction * stepSize);

        rayleighAccum += transmittance * rayleighCoeff * rayleighDensity * stepSize;
        mieAccum += transmittance * mieCoeff * mieDensity * stepSize;
    }

    //----------------------------------------------------------
    // 位相関数を適用
    //----------------------------------------------------------
    vec3 rayleighPhase = RayleighPhase(cosTheta) * rayleighAccum;
    vec3 miePhaseVal   = MiePhase(cosTheta, mieAnisotropy) * mieAccum;

    vec3 skyColor = sunIntensity * lightColor.rgb * lightColor.w * (rayleighPhase + miePhaseVal);

    //----------------------------------------------------------
    // 太陽円盤
    //----------------------------------------------------------
    float sunDisk = smoothstep(sunDiskSize, sunDiskSize * 0.5, acos(clamp(cosTheta, -1.0, 1.0)));
    skyColor += sunDisk * sunIntensity * lightColor.rgb * lightColor.w * transmittance;

    //----------------------------------------------------------
    // 地面方向は地面カラーで塗りつぶす
    //----------------------------------------------------------
    if(rayDir.y < 0.0) {
        float blend = smoothstep(-0.2, 0.05, rayDir.y);
        skyColor    = mix(groundColor.rgb, skyColor, blend);
    }

    gl_FragColor = vec4(skyColor, 1.0);
}

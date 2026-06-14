//--------------------------------------------------------------
//! @file   Sky.ps.hlsl
//! @brief  大気散乱用ピクセルシェーダー
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma pack_matrix(row_major)

static const float PI = 3.14159265358979f;

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
//! @brief 大気散乱定数バッファ (b4)
//--------------------------------------------------------------
cbuffer CBufferSky : register(b4)
{
    float rayleighScattering;
    float mieScattering;
    float mieAnisotropy;
    float sunIntensity;

    float atmosphereHeight;
    float planetRadius;
    float sunDiskSize;
    float padding0;

    float4 groundColor;
    float4 sunDirection; // xyz: 太陽方向（ライトの逆方向）
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief レイリー位相関数
//--------------------------------------------------------------
float RayleighPhase(float cosTheta)
{
    return (3.0f / (16.0f * PI)) * (1.0f + cosTheta * cosTheta);
}

//--------------------------------------------------------------
//! @brief ミー位相関数（Henyey-Greenstein）
//--------------------------------------------------------------
float MiePhase(float cosTheta, float g)
{
    float g2 = g * g;
    float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    return (1.0f / (4.0f * PI)) * ((1.0f - g2) / pow(abs(denom), 1.5f));
}

//--------------------------------------------------------------
//! @brief ピクセルシェーダーメイン
//--------------------------------------------------------------
float4 PSMain(PSInput input) : SV_TARGET
{
    //----------------------------------------------------------
    // NDC座標からワールド空間の視線方向を逆算
    //----------------------------------------------------------
    float2 ndc = input.uv * 2.0f - 1.0f;

    // リバースZの最遠点（z=0）をワールド座標に変換
    float4 worldPos = mul(float4(ndc, 0.0f, 1.0f), invViewProj);
    worldPos /= worldPos.w;

    float3 rayDir = normalize(worldPos.xyz - cameraPos.xyz);

    //----------------------------------------------------------
    // 大気散乱の計算（シーン原点を地表とするローカルスケール）
    //----------------------------------------------------------
    float3 sunDir = normalize(sunDirection.xyz);

    float cosTheta = dot(rayDir, sunDir);

    // 上方向ほど大気を通る距離が短い、地平線方向ほど長い
    // rayDir.y が小さい（地平線付近）ときに距離が伸びるようにする
    float cosAngle = max(rayDir.y, 0.001f); // ゼロ除算防止
    float rayLength = min(atmosphereHeight / cosAngle, atmosphereHeight * 50.0f);

    //----------------------------------------------------------
    // レイマーチング（サンプル数16）
    //----------------------------------------------------------
    const int SAMPLE_COUNT = 16;
    float stepSize = rayLength / float(SAMPLE_COUNT);

    float3 rayleighCoeff = float3(5.8e-6f, 13.5e-6f, 33.1e-6f) * rayleighScattering;
    float mieCoeff = 2.0e-6f * mieScattering;

    float3 rayleighAccum = float3(0.0f, 0.0f, 0.0f);
    float3 mieAccum = float3(0.0f, 0.0f, 0.0f);
    float3 transmittance = float3(1.0f, 1.0f, 1.0f);

    for (int i = 0; i < SAMPLE_COUNT; ++i)
    {
        // サンプル位置の高度（シーン原点=地表からの高さ）
        float sampleHeight = (float(i) + 0.5f) * stepSize * cosAngle;
        float heightRatio = saturate(sampleHeight / atmosphereHeight);

        float rayleighDensity = exp(-heightRatio * 8.0f);
        float mieDensity = exp(-heightRatio * 1.2f);

        float3 rayleighExtinction = rayleighCoeff * rayleighDensity;
        float3 mieExtinction = mieCoeff * mieDensity * 1.1f;

        float3 extinction = rayleighExtinction + mieExtinction;

        transmittance *= exp(-extinction * stepSize);

        rayleighAccum += transmittance * rayleighCoeff * rayleighDensity * stepSize;
        mieAccum += transmittance * mieCoeff * mieDensity * stepSize;
    }

    //----------------------------------------------------------
    // 位相関数を適用
    //----------------------------------------------------------
    float3 rayleighPhase = RayleighPhase(cosTheta) * rayleighAccum;
    float3 miePhaseVal = MiePhase(cosTheta, mieAnisotropy) * mieAccum;

    float3 skyColor = sunIntensity * lightColor.rgb * lightColor.w
                    * (rayleighPhase + miePhaseVal);

    //----------------------------------------------------------
    // 太陽円盤
    //----------------------------------------------------------
    float sunDisk = smoothstep(sunDiskSize, sunDiskSize * 0.5f, acos(clamp(cosTheta, -1.0f, 1.0f)));
    skyColor += sunDisk * sunIntensity * lightColor.rgb * lightColor.w * transmittance;

    //----------------------------------------------------------
    // 地面方向は地面カラーで塗りつぶす
    //----------------------------------------------------------
    if (rayDir.y < 0.0f)
    {
        float blend = smoothstep(-0.2f, 0.05f, rayDir.y); // 範囲を広げる
        skyColor = lerp(groundColor.rgb, skyColor, blend);
    }

    // 露出補正（小さい値を可視範囲にスケール）
    skyColor *= 1.0f;

    return float4(skyColor, 1.0f);
}

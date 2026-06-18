//--------------------------------------------------------------
//! @file   Water.ps.hlsl
//! @brief  水面ピクセルシェーダー
//!         - Gerstner波 + FBMによる動的法線生成
//!         - フレネル反射（スカイカラー代用）
//!         - 浅瀬/深部の色補間
//!         - Blinn-Phong スペキュラハイライト
//!         - シャドウマップ対応（t8/s8）
//!         - VSはModelStatic.vs.hlslをそのまま流用
//!         - shadowCoordはPS内でworldPosから計算
//--------------------------------------------------------------

//==============================================================
// 定数バッファ定義
// ConstantBuffer.hpp の構造体と対応させる
//==============================================================

//--------------------------------------------------------------
// b0 : CBufferScene
//--------------------------------------------------------------
cbuffer CBufferScene : register(b0)
{
    matrix view;
    matrix projection;
    matrix viewProj;
    matrix invViewProj;
    matrix lightViewProj;
    float4 lightDir;
    float4 lightColor; // xyz: 色(linear), w: 強度
    float4 cameraPos; // xyz: カメラワールド座標
};

//--------------------------------------------------------------
// b5 : CBufferWater
//--------------------------------------------------------------
cbuffer CBufferWater : register(b5)
{
    float time;
    float waveSpeed;
    float waveScale;
    float fresnelPower;

    float4 shallowColor; // xyz: 浅瀬の色
    float4 deepColor; // xyz: 深部の色
};

//==============================================================
// テクスチャ / サンプラー
//==============================================================

// t8 : シャドウマップ
Texture2D shadowMap : register(t8);
SamplerComparisonState shadowSampler : register(s8);

//==============================================================
// 頂点シェーダーからの入力
// ModelStatic.vs.hlsl の VSOutput と対応させる
//==============================================================
struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    // shadowCoord は VS出力に含まれないため、
    // PS内で worldPos から再計算する
};

//==============================================================
// ユーティリティ関数
//==============================================================

//--------------------------------------------------------------
//! @brief  ハッシュ関数（擬似乱数生成用）
//--------------------------------------------------------------
float hash(float2 p)
{
    p = frac(p * float2(127.1, 311.7));
    p += dot(p, p + 19.19);
    return frac(p.x * p.y);
}

//--------------------------------------------------------------
//! @brief  スムーズノイズ（バイリニア補間）
//--------------------------------------------------------------
float smoothNoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0 - 2.0 * f); // smoothstep

    float a = hash(i + float2(0, 0));
    float b = hash(i + float2(1, 0));
    float c = hash(i + float2(0, 1));
    float d = hash(i + float2(1, 1));

    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

//--------------------------------------------------------------
//! @brief  FBM（フラクタルブラウン運動）
//!         複数オクターブのノイズを重ねて細かい波紋を表現
//--------------------------------------------------------------
float fbm(float2 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    float2 shift = float2(100.0, 100.0);

    [unroll]
    for (int i = 0; i < 5; i++)
    {
        value += amplitude * smoothNoise(p);
        p = p * 2.0 + shift;
        amplitude *= 0.5;
    }
    return value;
}

//--------------------------------------------------------------
//! @brief  FBMから法線を生成（差分法）
//!         隣接2点のFBM値の差を使って勾配を求める
//--------------------------------------------------------------
float3 fbmNormal(float2 uv, float delta)
{
    float hC = fbm(uv);
    float hR = fbm(uv + float2(delta, 0.0));
    float hU = fbm(uv + float2(0.0, delta));

    // 接線ベクトルから法線を構築（Y軸が上方向）
    float3 tangentX = float3(delta, (hR - hC) * 2.0, 0.0);
    float3 tangentZ = float3(0.0, (hU - hC) * 2.0, delta);
    return normalize(cross(tangentZ, tangentX));
}

//--------------------------------------------------------------
//! @brief  Gerstner波（正弦波ベースの法線）
//!         方向・波長・振幅・位相速度で制御する物理ベース波形
//--------------------------------------------------------------
float3 gerstnerNormal(float2 worldXZ, float t)
{
    float3 normal = float3(0.0, 1.0, 0.0);

    // 波パラメータ定義（方向, 波数k, 振幅A, 速度omega）
    // 複数の波を重ねてリアルな海面を表現
    const float2 dirs[4] =
    {
        float2(1.0, 0.3),
        float2(-0.5, 1.0),
        float2(0.7, -0.8),
        float2(-0.9, -0.2),
    };
    const float wavelengths[4] = { 8.0, 5.0, 3.0, 2.0 };
    const float amplitudes[4] = { 0.06, 0.04, 0.02, 0.01 };

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        float2 d = normalize(dirs[i]);
        float k = 6.28318 / wavelengths[i]; // 2π / λ
        float omega = sqrt(9.8 * k); // 分散関係
        float A = amplitudes[i];
        float phase = k * dot(d, worldXZ) - omega * t;

        // Gerstner波の法線寄与（∂y/∂x, ∂y/∂z の傾き）
        float sinP = sin(phase);
        normal.x -= d.x * k * A * sinP;
        normal.z -= d.y * k * A * sinP;
    }

    return normalize(normal);
}

//--------------------------------------------------------------
//! @brief  PCFシャドウサンプリング（3x3カーネル）
//!         worldPos から PS内で shadowCoord を計算する
//--------------------------------------------------------------
float SampleShadow(float3 worldPos)
{
    // lightViewProj で ライト空間へ変換（b0 の値をそのまま使う）
    float4 shadowCoord = mul(float4(worldPos, 1.0), lightViewProj);

    // パースペクティブ除算
    float3 projCoord = shadowCoord.xyz / shadowCoord.w;

    // クリップ範囲外は影なし
    if (projCoord.x < 0.0 || projCoord.x > 1.0 ||
        projCoord.y < 0.0 || projCoord.y > 1.0)
        return 1.0;

    float shadow = 0.0;
    float texelSize = 1.0 / 2048.0;

    [unroll]
    for (int y = -1; y <= 1; y++)
    {
        [unroll]
        for (int x = -1; x <= 1; x++)
        {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.SampleCmpLevelZero(
                shadowSampler,
                projCoord.xy + offset,
                projCoord.z
            );
        }
    }
    return shadow / 9.0;
}

//==============================================================
// メインエントリ
//==============================================================
float4 PSMain(PSInput input) : SV_TARGET
{
    //----------------------------------------------------------
    // 1. UVスクロール（waveSpeed / waveScale を CBufferWater から適用）
    //----------------------------------------------------------
    float2 scrolledUV = input.uv * waveScale + float2(time * waveSpeed, time * waveSpeed * 0.7);

    //----------------------------------------------------------
    // 2. 動的法線の合成
    //    FBM（細かい波紋）＋ Gerstner（大きなうねり）をブレンド
    //----------------------------------------------------------
    const float fbmDelta = 0.01;
    float3 fbmN = fbmNormal(scrolledUV, fbmDelta);

    // Gerstner波はワールドXZ座標を使う（UV歪みなしで一定周期）
    float3 gerstnerN = gerstnerNormal(input.worldPos.xz, time * waveSpeed);

    // FBMは細部、Gerstnerは大波——重み付けで合成
    float3 N = normalize(fbmN * 0.5 + gerstnerN * 0.5);

    //----------------------------------------------------------
    // 3. 視線ベクトル
    //----------------------------------------------------------
    float3 V = normalize(cameraPos.xyz - input.worldPos);

    //----------------------------------------------------------
    // 4. フレネル係数（Schlick近似）
    //    浅い角度（水平方向）ほど強く反射する
    //----------------------------------------------------------
    float NdotV = saturate(dot(N, V));
    float fresnel = pow(1.0 - NdotV, fresnelPower);

    //----------------------------------------------------------
    // 5. 深度近似による浅瀬/深部カラーブレンド
    //    スカイ方向の法線Y成分を透明度の代わりに使う
    //    （深度バッファなし構成でも視覚的に機能する）
    //----------------------------------------------------------
    float depthFactor = saturate(1.0 - N.y * 1.5);
    float3 waterColor = lerp(shallowColor.xyz, deepColor.xyz, depthFactor);

    //----------------------------------------------------------
    // 6. 拡散反射（Lambert）
    //----------------------------------------------------------
    float3 L = normalize(-lightDir.xyz);
    float NdotL = saturate(dot(N, L));
    float3 diffuse = waterColor * lightColor.xyz * lightColor.w * NdotL;

    //----------------------------------------------------------
    // 7. スペキュラハイライト（Blinn-Phong）
    //    水面は鏡面性が高いので shininess を大きく取る
    //----------------------------------------------------------
    float3 H = normalize(L + V);
    float NdotH = saturate(dot(N, H));
    float shininess = 256.0;
    float3 specular = lightColor.xyz * lightColor.w * pow(NdotH, shininess);

    //----------------------------------------------------------
    // 8. 反射カラー（スカイカラー代用）
    //    本来は CubeMap / SSR を使うが、ここでは
    //    ライト色とフレネルで擬似的なスカイを表現
    //    → より良い結果を得たい場合は t2 に CubeMap を渡して
    //      texCube.Sample(sampler, reflect(-V, N)) にすること
    //----------------------------------------------------------
    float3 skyColor = lightColor.xyz * 0.5 + float3(0.2, 0.4, 0.6);
    float3 reflectColor = skyColor * fresnel;

    //----------------------------------------------------------
    // 9. シャドウ
    //----------------------------------------------------------
    float shadow = SampleShadow(input.worldPos);

    //----------------------------------------------------------
    // 10. 最終合成
    //     影はdiffuseとspecularに乗算、反射は影響させない
    //----------------------------------------------------------
    float3 ambient = waterColor * 0.08;
    float3 color = ambient
                   + (diffuse + specular) * shadow
                   + reflectColor;

    // アルファ：フレネルが強いほど不透明（視線が浅いほど反射し白く見える）
    float alpha = lerp(0.75, 1.0, fresnel);

    return float4(color, alpha);
}

//--------------------------------------------------------------
//! @file   Model.ps.hlsl
//! @brief  3Dモデル用のピクセルシェーダ
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma pack_matrix(row_major)

// 円周率を定義
static const float PI = 3.14159265358979323846f;

//--------------------------------------------------------------
//! @brief シーン用定数バッファ
//--------------------------------------------------------------
cbuffer CBufferScene : register(b0)
{
    matrix view;
    matrix projection;
    matrix viewProj;
    matrix lightViewProj; // ライト空間のViewProjection行列
    float4 lightDir; // xyz: ライト方向（正規化済み）
    float4 lightColor; // xyz: ライトの色, w: 未使用
    float4 cameraPos; // xyz: カメラのワールド座標, w: 未使用
};

//--------------------------------------------------------------
//! @brief モデル位置定数バッファ
//--------------------------------------------------------------
cbuffer CBufferTransform : register(b1)
{
    matrix world;
};

//--------------------------------------------------------------
//! @brief マテリアル定数バッファ
//--------------------------------------------------------------
cbuffer CBufferMaterial : register(b2)
{
    float4 baseColor;
    float3 emissive;
    float metallic;
    float roughness;
    float specular;
    float4 padding;
};

//--------------------------------------------------------------
//! @brief アルベドテクスチャ (t0)
//--------------------------------------------------------------
Texture2D albedoTexture : register(t0);
//--------------------------------------------------------------
//! @brief シャドウマップ (t8)
//--------------------------------------------------------------
Texture2D shadowMap : register(t8);
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
//! @brief PCFシャドウサンプリング
//! @return 遮蔽量 0.0f(暗) - 1.0f(明)
//--------------------------------------------------------------
float GetShadowPCF(float3 worldPos)
{
    //----------------------------------------------------------
    // ワールド座標をライト空間に変換
    //----------------------------------------------------------
    float4 lightSpace = mul(float4(worldPos, 1.0f), lightViewProj);

    //----------------------------------------------------------
    // クリップ座標をUV座標に変換
    // x: [-1, 1] → [0, 1]
    // y: [-1, 1] → [1, 0] （DirectXはY反転）
    //----------------------------------------------------------
    float2 uv = lightSpace.xy * float2(0.5f, -0.5f) + 0.5f;

    // UV範囲外は影なし
    if (any(uv < 0.0f) || any(1.0f < uv))
        return 1.0f;

    // 深度値 + シャドウアクネ対策バイアス
    float depth = lightSpace.z + 0.001f;

    //----------------------------------------------------------
    // 3x3 PCF : 9サンプルの平均を取る
    //----------------------------------------------------------
    float shadow = 0.0f;
    float texelSize = 1.0f / 2048.0f; // SHADOW_MAP_SIZEに合わせる

    [unroll]
    for (int x = -1; x <= 1; x++)
    {
        [unroll]
        for (int y = -1; y <= 1; y++)
        {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.SampleCmpLevelZero(
                shadowSampler, uv + offset, depth);
        }
    }

    return shadow / 9.0f;
}

//--------------------------------------------------------------
//! @brief 法線分布関数 (NDF): GGX / Trowbridge-Reitz
//! @param NdotH  法線とハーフベクトルの内積
//! @param a      粗さの2乗 (roughness^2)
//! @return マイクロファセットの分布密度
//! @note  粗さが高いほど分布が広がり、ぼやけた鏡面反射になる
//--------------------------------------------------------------
float D_GGX(float NdotH, float a)
{
    float a2 = a * a;
    float denom = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;
    // D = a^2 / (PI * ((NdotH^2 * (a^2 - 1) + 1))^2)
    return a2 / (PI * denom * denom);
}

//--------------------------------------------------------------
//! @brief 幾何減衰関数 (GSF): Smith / Schlick-GGX
//! @param NdotV  法線と視線ベクトルの内積
//! @param NdotL  法線とライト方向の内積
//! @param k      roughness由来の係数 = (roughness+1)^2 / 8
//! @return 自己遮蔽・自己シャドウの減衰係数 [0, 1]
//! @note  視線側とライト側それぞれの遮蔽を掛け合わせる
//--------------------------------------------------------------
float G_Smith(float NdotV, float NdotL, float k)
{
    // Schlick-GGX: G1(v) = NdotV / (NdotV * (1-k) + k)
    float ggx1 = NdotV / (NdotV * (1.0f - k) + k); // 視線側
    float ggx2 = NdotL / (NdotL * (1.0f - k) + k); // ライト側
    return ggx1 * ggx2;
}

//--------------------------------------------------------------
//! @brief フレネル項: Schlick近似
//! @param cosTheta  視線とハーフベクトルの内積
//! @param F0        垂直入射時の反射率（誘電体=0.04、金属=アルベド色）
//! @return 視角に応じた反射率
//! @note  斜めから見るほど反射率が上がる（縁が光って見える現象）
//--------------------------------------------------------------
float3 F_Schlick(float cosTheta, float3 F0)
{
    // F = F0 + (1 - F0) * (1 - cosTheta)^5
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
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
    float3 albedo = albedoSample.rgb * baseColor.rgb; // テクスチャ × 定数色
    float met = metallic;
    float rough = max(roughness, 0.15f); // 0に近いと分母が発散するので下限を設ける

    //----------------------------------------------------------
    // 法線（現状は頂点法線をそのまま使用）
    // ノーマルマップを使う場合はここでTBN変換を行う
    //----------------------------------------------------------
    float3 N = normalize(input.normal);

    //----------------------------------------------------------
    // 照明計算に必要なベクトルを算出
    //   V: 視線ベクトル（ピクセル → カメラ）
    //   L: ライトベクトル（ピクセル → ライト）
    //   H: ハーフベクトル（VとLの中間）
    //----------------------------------------------------------
    float3 V = normalize(cameraPos.xyz - input.worldPos);
    float3 L = normalize(-lightDir.xyz); // lightDirは「ライトが向いている方向」なので反転
    float3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L)); // ランバート項（ライトの入射角）
    float NdotV = saturate(dot(N, V)) + 1e-5f; // ゼロ除算防止のためεを加算
    float NdotH = saturate(dot(N, H));
    float HdotV = saturate(dot(H, V)); // フレネル計算用

    //----------------------------------------------------------
    // Cook-Torrance BRDF の各項を計算
    //   BRDF = (D * G * F) / (4 * NdotV * NdotL)
    //----------------------------------------------------------
    float a = rough * rough; // α = roughness^2（GGXの慣習）
    float k = (rough + 1.0f) * (rough + 1.0f) / 8.0f; // 直接照明用のk（IBLとは別式）

    // F0: 垂直入射時の反射率
    //   誘電体（プラスチック等）: specular=0.5 → 0.08*0.5 = 0.04
    //   金属: アルベドをそのまま反射色として使う
    float3 F0 = lerp(0.08f * specular.xxx, albedo, met);

    float D = D_GGX(NdotH, a);
    float G = G_Smith(NdotV, NdotL, k);
    float3 F = F_Schlick(HdotV, F0);

    // 鏡面反射項
    float3 spec = (D * G * F) / (4.0f * NdotV * NdotL + 1e-5f);

    // 拡散反射項
    //   kD: エネルギー保存のため鏡面反射分(F)を引く
    //   金属は自由電子が光を吸収するため拡散反射なし (1-met)
    float3 kD = (1.0f - F) * (1.0f - met);
    float3 diff = kD * albedo / PI; // ランバート拡散

    //----------------------------------------------------------
    // シャドウ係数とライト放射輝度 (radiance) を取得
    //----------------------------------------------------------
    float shadow = GetShadowPCF(input.worldPos);
    float3 radiance = lightColor.rgb * lightColor.w; // 色 × 強度

    // 直接照明 = BRDF × radiance × NdotL × shadow
    float3 directLight = (diff + spec) * radiance * NdotL * shadow;

    //----------------------------------------------------------
    // アンビエント（IBLの代わりの定数環境光）
    // 本格的なIBLにする場合はキューブマップサンプリングに差し替える
    //----------------------------------------------------------
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo;

    //----------------------------------------------------------
    // 最終カラー合成
    //   ambient: 間接光の簡易近似
    //   directLight: 直接照明（拡散 + 鏡面）
    //   emissive: 自発光（ライティング非依存）
    //----------------------------------------------------------
    float3 finalColor = ambient + directLight + emissive;

    return float4(finalColor, baseColor.a * albedoSample.a);
}

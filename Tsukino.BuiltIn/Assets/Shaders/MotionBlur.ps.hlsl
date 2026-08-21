//--------------------------------------------------------------
//! @file   MotionBlur.ps.hlsl
//! @brief  オブジェクト速度バッファを使ったモーションブラー
//! @author 山﨑愛
//! @note   VSはTonemap.vs.hlslを共用する（頂点バッファ不要のフルスクリーン三角形）。
//!         G-Buffer5（速度）を読み、その方向へシーンカラーを平均する。
//!
//!         【既知の限界】
//!         中心ピクセルの速度だけで近傍を集める素朴なgatherなので、
//!         動く物体の内側に背景が引き込まれる方向にしか滲まない
//!         （物体が背景へはみ出して尾を引く絵にはならない）。
//!         改善するなら速度のdilation（NeighborMaxタイルパス）が次の一手。
//!         また深度を考慮した重み付けもしていないため、手前／奥が
//!         混ざるケースでは滲む。
//--------------------------------------------------------------

//--------------------------------------------------------------
//! @brief 入力テクスチャ
//--------------------------------------------------------------
Texture2D    sceneTexture : register(t0);     // シーンカラー（HDR）
SamplerState sceneSampler : register(s0);     // LinearClamp

Texture2D    velocityTexture : register(t15); // G-Buffer5（rg: 1フレームあたりのUV移動量）
SamplerState velocitySampler : register(s9);  // PointClamp

//--------------------------------------------------------------
//! @brief モーションブラーパラメータ (b8)
//--------------------------------------------------------------
cbuffer CBufferMotionBlur : register(b8)
{
    float strength;         // 速度ベクトルの倍率
    float maxBlurRadius;    // UV単位のブラー長クランプ
    float shutterScale;     // 可変フレームレート補正
    int   sampleCount;      // サンプル数
};

//! @brief サンプル数の上限（ConstantBuffer.hpp の kMotionBlurMaxSamples と一致させること）
static const int kMaxSamples = 16;

//! @brief これ以下の速度はブラーを掛けない（UV単位）
static const float kMinVelocity = 1e-5f;

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief ピクセル座標から 0〜1 の疑似乱数を作る
//! @note  サンプル位置を1タップ未満だけずらすためのジッター用。
//!        サンプル数が有限だと、ブラーが長いほどタップの間隔が開いて
//!        「元画像が何枚も重なったゴースト」に見えてしまう。
//!        ピクセルごとに位相をずらすと、その縞がフィルムグレイン状の
//!        ノイズに分解されて目立たなくなる（定番の手法）。
//--------------------------------------------------------------
float Hash(float2 pixel)
{
    return frac(sin(dot(pixel, float2(12.9898f, 78.233f))) * 43758.5453f);
}

float4 PSMain(PSInput input) : SV_TARGET
{
    //----------------------------------------------------------
    // フルスクリーン三角形のUVはTonemap/Lightingパスと同じ生成規則。
    // テクスチャ座標系（Y下向き）に合わせて反転する
    //----------------------------------------------------------
    float2 screenUV = float2(input.uv.x, 1.0f - input.uv.y);

    float4 center = sceneTexture.Sample(sceneSampler, screenUV);

    //----------------------------------------------------------
    // 速度を取得してスケール
    // 速度バッファには生のUV移動量しか入っていないので、
    // 強度とシャッター補正はここで掛ける
    //----------------------------------------------------------
    float2 velocity = velocityTexture.Sample(velocitySampler, screenUV).rg;
    velocity *= strength * shutterScale;

    float speed = length(velocity);

    //----------------------------------------------------------
    // 静止しているピクセル（背景・フォワードパスの描画結果を含む）は
    // そのまま返す。速度バッファは0クリアされているので分岐が効く。
    //----------------------------------------------------------
    if (speed < kMinVelocity)
        return center;

    // ブラー長をクランプ（高速移動時に画面全体が溶けるのを防ぐ）
    velocity = velocity * (min(speed, maxBlurRadius) / speed);

    //----------------------------------------------------------
    // 速度方向へ中心対称にサンプルして平均する
    //   t = -0.5 .. +0.5 の範囲
    //
    // ジッターでタップ位置を1ステップ未満だけずらし、
    // サンプル数の少なさによるゴースト（縞）をノイズへ変える
    //----------------------------------------------------------
    int    taps   = clamp(sampleCount, 1, kMaxSamples);
    float  jitter = Hash(input.position.xy) - 0.5f;
    float4 sum    = center;
    float  weight = 1.0f;

    [loop]
    for (int i = 0; i < kMaxSamples; ++i)
    {
        if (i >= taps)
            break;

        // taps==1 のときは 0 除算を避けて中心のみ
        float t = (taps > 1) ? (((float)i + jitter) / (float)(taps - 1)) - 0.5f : 0.0f;

        float2 sampleUV = screenUV + velocity * t;

        sum += sceneTexture.Sample(sceneSampler, sampleUV);
        weight += 1.0f;
    }

    return sum / weight;
}

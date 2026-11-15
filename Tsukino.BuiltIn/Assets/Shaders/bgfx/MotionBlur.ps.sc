$input v_texcoord0

//--------------------------------------------------------------
//! @file   MotionBlur.ps.sc
//! @brief  オブジェクト速度バッファを使ったモーションブラー（MotionBlur.ps.hlsl の bgfx 版）
//! @note   頂点シェーダーは Tonemap.vs を共用するフルスクリーン三角形。
//!         G-Buffer の速度（5枚目）を読み、その方向へシーンカラーを平均する。
//!
//!         【既知の限界】（HLSL 版と同じ）
//!         中心ピクセルの速度だけで近傍を集める素朴な gather なので、
//!         動く物体の内側に背景が引き込まれる方向にしか滲まない。
//!         深度を考慮した重み付けもしていないので、手前／奥が混ざるケースでは滲む
//--------------------------------------------------------------
#include <bgfx_shader.sh>

SAMPLER2D(s_motionBlurScene, 0);       // シーンカラー（HDR）。LinearClamp
SAMPLER2D(s_motionBlurVelocity, 1);    // G-Buffer の速度（rg: 1フレームあたりの UV 移動量）。PointClamp

// モーションブラーのパラメータ（DX11 版の b7）。C++ 側がサンプル数も float にして送る
uniform vec4 u_motionBlur;
#define blurStrength      u_motionBlur.x                // 速度ベクトルの倍率
#define blurMaxRadius     u_motionBlur.y                // UV 単位のブラー長クランプ
#define blurShutterScale  u_motionBlur.z                // 可変フレームレート補正
#define blurSampleCount   int(u_motionBlur.w + 0.5)     // サンプル数

// サンプル数の上限（ConstantBuffer.hpp の kMotionBlurMaxSamples と一致させること）
#define kMaxSamples 16

// これ以下の速度はブラーを掛けない（UV 単位）
#define kMinVelocity 1e-5

//--------------------------------------------------------------
//! @brief  ピクセル座標から 0〜1 の疑似乱数を作る
//! @note   サンプル位置を1タップ未満だけずらすためのジッター用。
//!         サンプル数が有限だと、ブラーが長いほどタップの間隔が開いて
//!         「元画像が何枚も重なったゴースト」に見えてしまう。
//!         ピクセルごとに位相をずらすと、その縞がフィルムグレイン状のノイズに分解されて目立たなくなる
//--------------------------------------------------------------
float Hash(vec2 pixel)
{
    return fract(sin(dot(pixel, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    // テクスチャ座標（Y 下向き）に合わせて反転する
    vec2 screenUV = vec2(v_texcoord0.x, 1.0 - v_texcoord0.y);

    vec4 center = texture2DLod(s_motionBlurScene, screenUV, 0.0);

    //----------------------------------------------------------
    // 速度を取得してスケール。速度バッファには生の UV 移動量しか入っていないので、
    // 強度とシャッター補正はここで掛ける
    //----------------------------------------------------------
    vec2 velocity = texture2DLod(s_motionBlurVelocity, screenUV, 0.0).rg;
    velocity *= blurStrength * blurShutterScale;

    float speed = length(velocity);

    // 静止しているピクセル（背景・フォワードパスの描画結果を含む）はそのまま返す
    if(speed < kMinVelocity) {
        gl_FragColor = center;
        return;
    }

    // ブラー長をクランプ（高速移動時に画面全体が溶けるのを防ぐ）
    velocity = velocity * (min(speed, blurMaxRadius) / speed);

    //----------------------------------------------------------
    // 速度方向へ中心対称にサンプルして平均する（t = -0.5 .. +0.5）。
    // ジッターでタップ位置を1ステップ未満だけずらし、
    // サンプル数の少なさによるゴースト（縞）をノイズへ変える
    //----------------------------------------------------------
    int   taps   = clamp(blurSampleCount, 1, kMaxSamples);
    float jitter = Hash(gl_FragCoord.xy) - 0.5;
    vec4  sum    = center;
    float weight = 1.0;

    for(int i = 0; i < kMaxSamples; ++i) {
        if(i >= taps)
            break;

        // taps == 1 のときは 0 除算を避けて中心のみ
        float t = (taps > 1) ? ((float(i) + jitter) / float(taps - 1)) - 0.5 : 0.0;

        sum += texture2DLod(s_motionBlurScene, screenUV + velocity * t, 0.0);
        weight += 1.0;
    }

    gl_FragColor = sum / weight;
}

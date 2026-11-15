$output v_texcoord0, v_color0

//--------------------------------------------------------------
//! @file   AmbientParticle.vs.sc
//! @brief  環境パーティクル用の頂点シェーダー（AmbientParticle.vs.hlsl の bgfx 版）
//! @note   頂点バッファもインデックスバッファも持たず、頂点番号（gl_VertexID）だけから
//!         粒子1粒 ＝ 6頂点（三角形2枚のビルボード）を作る。
//!         位置・大きさ・輝度・速さはすべて粒子番号のハッシュから決まるので、
//!         CPU 側にも GPU 側にも粒子1個あたりのメモリを持たない。
//!         粒子の座標はカメラを中心としたボリュームで折り返すので、
//!         有限の粒子数でカメラがどこへ動いても空間が埋まり続ける。
//!         v_texcoord0: 板の中心を原点とした -1〜1 の座標, v_color0: rgb 色×輝度, a フェード量
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"

// 環境パーティクルの定数（DX11 版の b9。C++ の CBufferAmbientParticle）
#define TSUKINO_AMBIENT_PARTICLE_VEC4_COUNT 6    // sizeof(CBufferAmbientParticle) / 16

uniform vec4 u_ambientParticle[TSUKINO_AMBIENT_PARTICLE_VEC4_COUNT];

#define volumeParams u_ambientParticle[0]    // xyz: ボリュームの一辺の長さ, w: 経過時間（秒）
#define fadeParams   u_ambientParticle[1]    // x: 境界フェード開始比率(0〜1), y: 近接フェード距離, z: 乱数シード
#define sizeParams   u_ambientParticle[2]    // x: 最小サイズ（半径）, y: 最大サイズ（半径）, z: 最小輝度, w: 最大輝度
#define driftParams  u_ambientParticle[3]    // xyz: 一定ドリフト速度, w: 揺らぎの角速度（rad/秒）
#define swayParams   u_ambientParticle[4]    // x: 揺らぎの振幅, y: 速度倍率の下限, z: 速度倍率の上限, w: きらめきの強さ
#define colorParams  u_ambientParticle[5]    // xyz: 粒子色（linear）, w: 全体の強度

#define TWO_PI 6.28318530718

//--------------------------------------------------------------
//! @brief  32ビット整数をビット撹拌して別の整数へ写す
//! @note   連番（粒子番号）を入れても隣どうしの相関が残らないことが重要
//--------------------------------------------------------------
uint HashU32(uint xIn)
{
    uint x = xIn;
    x ^= x >> 16u;
    x *= 0x7feb352du;
    x ^= x >> 15u;
    x *= 0x846ca68bu;
    x ^= x >> 16u;
    return x;
}

//--------------------------------------------------------------
//! @brief  整数から 0.0〜1.0 の擬似乱数を作る
//! @note   下位24ビットだけを使う。float の仮数部が24ビットなので、これで割ると分布に偏りが出ない
//--------------------------------------------------------------
float Rand01(uint x)
{
    return float(HashU32(x) & 0x00ffffffu) * (1.0 / 16777216.0);
}

void main()
{
    //----------------------------------------------------------
    // 頂点番号を「何粒目の・どの角か」へ分解する
    //----------------------------------------------------------
    uint id            = uint(gl_VertexID);
    uint particleIndex = id / 6u;
    uint cornerIndex   = id % 6u;

    // 1粒につき乱数を5本引くので、番号を8刻みにして種の衝突を避ける
    uint seed = uint(fadeParams.z) + particleIndex * 8u;

    // ボリューム内のどこに居るか（0〜1をボリュームの大きさへ引き伸ばす）
    vec3 unitPos = vec3(Rand01(seed + 0u), Rand01(seed + 1u), Rand01(seed + 2u));

    //----------------------------------------------------------
    // 層（0 = 大きく暗くゆっくり ⇔ 1 = 小さく明るく速い）。
    // 乱数を1本だけ引いて3つのパラメータを同じ向きに補間するのが要点。
    // 別々の乱数にすると「大きくて速くて明るい粒」が混ざって画が汚れる
    //----------------------------------------------------------
    float layer      = Rand01(seed + 3u);
    float radius     = mix(sizeParams.y, sizeParams.x, layer);
    float brightness = mix(sizeParams.z, sizeParams.w, layer);
    float speedScale = mix(swayParams.y, swayParams.z, layer);

    // 揺らぎときらめきの位相。粒子ごとにずらして一斉に動くのを防ぐ
    float phase = Rand01(seed + 4u) * TWO_PI;

    float time  = volumeParams.w;
    float omega = driftParams.w;

    //----------------------------------------------------------
    // 一定のドリフトに、軸ごとに周期をずらしたサインの揺らぎを重ねる。
    // 周期を互いに割り切れない倍率でずらすと、軌道が閉じずに漂って見える
    //----------------------------------------------------------
    vec3 drift = driftParams.xyz * (speedScale * time);
    vec3 sway  = vec3(sin(time * omega + phase), sin(time * omega * 0.73 + phase * 1.7), cos(time * omega * 0.61 + phase * 2.3)) * swayParams.x;

    vec3 rawPos = unitPos * volumeParams.xyz + drift + sway;

    //----------------------------------------------------------
    // カメラを中心にボリュームで折り返す（この技法の要）。
    // fract() は負数でも0〜1を返すので、これだけで座標がボリュームの剰余に畳み込まれる
    //----------------------------------------------------------
    vec3 halfSize = volumeParams.xyz * 0.5;
    vec3 origin   = cameraPos.xyz - halfSize;
    vec3 worldPos = origin + fract((rawPos - origin) / volumeParams.xyz) * volumeParams.xyz;

    //----------------------------------------------------------
    // 境界フェード。折り返しでは座標が不連続に飛ぶので、飛ぶ瞬間に粒子が消えていないと
    // ポップとして見えてしまう。ボリュームの面までに必ず0になるよう減衰させる
    //----------------------------------------------------------
    vec3  edgeRatio = abs(worldPos - cameraPos.xyz) / max(halfSize, vec3_splat(1.0e-4));
    float edgeMax   = max(edgeRatio.x, max(edgeRatio.y, edgeRatio.z));
    float edgeFade  = 1.0 - smoothstep(fadeParams.x, 1.0, edgeMax);

    // 近接フェード（目の前の1粒が画面を覆ってしまうのを防ぐ）
    float viewDist = length(worldPos - cameraPos.xyz);
    float nearFade = smoothstep(0.0, max(fadeParams.y, 1.0e-4), viewDist);

    // きらめき（揺らぎとは違う周期で明滅させてちらつきを作る）
    float twinkle = mix(1.0 - swayParams.w, 1.0, 0.5 + 0.5 * sin(time * omega * 1.9 + phase * 3.1));

    //----------------------------------------------------------
    // ビルボード。このエンジンは行ベクトル規約（mul(v, view)）なので、ビュー行列の
    // 「列」がそのままカメラの右・上ベクトルになる（u_scene[0]〜[2] がビュー行列の行）
    //----------------------------------------------------------
    vec3 camRight = vec3(u_scene[0].x, u_scene[1].x, u_scene[2].x);
    vec3 camUp    = vec3(u_scene[0].y, u_scene[1].y, u_scene[2].y);

    // 板1枚（三角形2枚）ぶんの角。番号 0〜5 が (-1,-1) (1,-1) (-1,1) / (-1,1) (1,-1) (1,1)
    vec2 corner = vec2((cornerIndex == 1u || cornerIndex == 4u || cornerIndex == 5u) ? 1.0 : -1.0,
                       (cornerIndex == 2u || cornerIndex == 3u || cornerIndex == 5u) ? 1.0 : -1.0);
    vec3 offset = (camRight * corner.x + camUp * corner.y) * radius;

    gl_Position = mul(vec4(worldPos + offset, 1.0), viewProj);
    v_texcoord0 = corner;

    // ブレンドは SRC_ALPHA / ONE の加算なので、色には事前乗算しない
    v_color0 = vec4(colorParams.rgb * (colorParams.w * brightness * twinkle), edgeFade * nearFade);
}

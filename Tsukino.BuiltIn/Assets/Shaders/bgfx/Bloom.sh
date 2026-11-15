//--------------------------------------------------------------
//! @file   Bloom.sh
//! @brief  ブルームパスの共通定義（Bloom.hlsli の bgfx 版。定数・入力・サンプリング）
//! @note   頂点シェーダーは Tonemap.vs を共用するフルスクリーン三角形。
//!         縮小は13タップ、拡大は3x3のテントフィルタ。縮小を 1/2 ずつ重ねるので、
//!         少ないタップでも画面の広い範囲まで滲みが届く。
//!         定数（DX11 版の b11。C++ の CBufferBloom）は vec4 の配列 u_bloom
//--------------------------------------------------------------
#ifndef TSUKINO_BLOOM_SH
#define TSUKINO_BLOOM_SH

#define TSUKINO_BLOOM_VEC4_COUNT 3    // sizeof(CBufferBloom) / 16

uniform vec4 u_bloom[TSUKINO_BLOOM_VEC4_COUNT];

#define bloomTexelSize u_bloom[0].xy     // 読む側の1テクセルの UV サイズ
#define bloomThreshold u_bloom[0].z      // 明部抽出の閾値
#define bloomSoftKnee  u_bloom[0].w      // 閾値付近をなだらかにする幅
#define bloomTint      u_bloom[1].xyz    // 出力に掛ける色
#define bloomIntensity u_bloom[1].w      // 出力に掛ける強さ
#define bloomRadius    u_bloom[2].x      // 拡大時のぼかし半径の倍率

// 読む側の絵（シーン、または1つ前の段）。サンプラーは LinearClamp
SAMPLER2D(s_bloomSource, 0);

//--------------------------------------------------------------
//! @brief  フルスクリーン三角形の UV をテクスチャ座標（Y 下向き）へ直す
//--------------------------------------------------------------
vec2 ToTextureUV(vec2 uv)
{
    return vec2(uv.x, 1.0 - uv.y);
}

//--------------------------------------------------------------
//! @brief  13タップの縮小フィルタ
//! @note   中央の2x2ブロックを重く、外周の4ブロックを軽く平均する。
//!         単純な2x2平均よりちらつき（細い光る線が縮小段ごとに出たり消えたりする）が少ない
//--------------------------------------------------------------
vec3 Downsample13(vec2 uv)
{
    vec2 t = bloomTexelSize;

    vec3 a = texture2D(s_bloomSource, uv + t * vec2(-2.0, -2.0)).rgb;
    vec3 b = texture2D(s_bloomSource, uv + t * vec2( 0.0, -2.0)).rgb;
    vec3 c = texture2D(s_bloomSource, uv + t * vec2( 2.0, -2.0)).rgb;
    vec3 d = texture2D(s_bloomSource, uv + t * vec2(-2.0,  0.0)).rgb;
    vec3 e = texture2D(s_bloomSource, uv).rgb;
    vec3 f = texture2D(s_bloomSource, uv + t * vec2( 2.0,  0.0)).rgb;
    vec3 g = texture2D(s_bloomSource, uv + t * vec2(-2.0,  2.0)).rgb;
    vec3 h = texture2D(s_bloomSource, uv + t * vec2( 0.0,  2.0)).rgb;
    vec3 i = texture2D(s_bloomSource, uv + t * vec2( 2.0,  2.0)).rgb;
    vec3 j = texture2D(s_bloomSource, uv + t * vec2(-1.0, -1.0)).rgb;
    vec3 k = texture2D(s_bloomSource, uv + t * vec2( 1.0, -1.0)).rgb;
    vec3 l = texture2D(s_bloomSource, uv + t * vec2(-1.0,  1.0)).rgb;
    vec3 m = texture2D(s_bloomSource, uv + t * vec2( 1.0,  1.0)).rgb;

    vec3 result = (j + k + l + m) * 0.125;
    result += (a + b + d + e) * 0.03125;
    result += (b + c + e + f) * 0.03125;
    result += (d + e + g + h) * 0.03125;
    result += (e + f + h + i) * 0.03125;
    return result;
}

#endif    // TSUKINO_BLOOM_SH

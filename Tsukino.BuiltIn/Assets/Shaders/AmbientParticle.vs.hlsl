//--------------------------------------------------------------
//! @file   AmbientParticle.vs.hlsl
//! @brief  環境パーティクル用頂点シェーダー
//! @author 山﨑愛
//--------------------------------------------------------------
// 頂点バッファもインデックスバッファも持たず、SV_VertexIDだけから
// 粒子1粒＝6頂点（三角形2枚のビルボード）を生成する。
// 位置・大きさ・輝度・速さはすべて粒子番号のハッシュから決まるため、
// CPU側にもGPU側にも粒子1個あたりのメモリを持たない。
// 粒子の座標はカメラを中心としたボリュームで折り返すので、
// 有限の粒子数でカメラがどこへ動いても空間が埋まり続ける。
#pragma pack_matrix(row_major)

//--------------------------------------------------------------
// 定数バッファ：シーン (b0)
// PBR.hlsliのCBufferSceneと同じ並び。使うのはview / viewProj / cameraPosだけ
// なので末尾のprevViewProjは宣言しない（前方のメンバの配置は変わらない）
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
// 定数バッファ：環境パーティクル (b10)
// ConstantBuffer.hpp の CBufferAmbientParticle と1バイト単位で一致させること
//--------------------------------------------------------------
cbuffer CBufferAmbientParticle : register(b10)
{
    float4 volumeParams;    // xyz: ボリュームの一辺の長さ, w: 経過時間（秒）
    float4 fadeParams;      // x: 境界フェード開始比率(0〜1), y: 近接フェード距離, z: 乱数シード, w: 予約
    float4 sizeParams;      // x: 最小サイズ（半径）, y: 最大サイズ（半径）, z: 最小輝度, w: 最大輝度
    float4 driftParams;     // xyz: 一定ドリフト速度, w: 揺らぎの角速度（rad/秒）
    float4 swayParams;      // x: 揺らぎの振幅, y: 速度倍率の下限, z: 速度倍率の上限, w: きらめきの強さ
    float4 colorParams;     // xyz: 粒子色（linear）, w: 全体の強度
};

//--------------------------------------------------------------
// 出力
//--------------------------------------------------------------
struct VSOutput
{
    float4 position : SV_POSITION;
    float2 local    : TEXCOORD0;    // 板の中心を原点とした -1〜1 の座標
    float4 color    : COLOR0;       // rgb: 色×輝度, a: フェード量
};

static const float TWO_PI = 6.28318530718f;

//--------------------------------------------------------------
// 板1枚（三角形2枚）ぶんの角の位置
// カリングは無効なので巻き方向は問わない
//--------------------------------------------------------------
static const float2 kCorners[6] = {
    float2(-1.0f, -1.0f), float2( 1.0f, -1.0f), float2(-1.0f,  1.0f),
    float2(-1.0f,  1.0f), float2( 1.0f, -1.0f), float2( 1.0f,  1.0f)
};

//--------------------------------------------------------------
//! 32ビット整数をビット撹拌して別の整数へ写します。
//! @param  [in] x 撹拌する値
//! @return 撹拌後の値
//! @note   連番（粒子番号）を入れても隣どうしの相関が残らないことが重要。
//!         乗算と右シフトXORを交互に掛けるだけなので非常に安価。
//--------------------------------------------------------------
uint HashU32(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

//--------------------------------------------------------------
//! 整数から 0.0〜1.0 の擬似乱数を作ります。
//! @param  [in] x 種となる整数
//! @return 0.0以上1.0未満の値
//! @note   下位24ビットだけを使う。floatの仮数部が24ビットなので、
//!         これで割ると分布に偏りが出ない。
//--------------------------------------------------------------
float Rand01(uint x)
{
    return float(HashU32(x) & 0x00ffffffu) * (1.0f / 16777216.0f);
}

//--------------------------------------------------------------
//! メイン関数
//! @param  [in] id 頂点ID
//! @return 変換後の頂点データ
//--------------------------------------------------------------
VSOutput VSMain(uint id : SV_VertexID)
{
    VSOutput output;

    //----------------------------------------------------------
    // 頂点IDを「何粒目の・どの角か」へ分解する
    //----------------------------------------------------------
    const uint particleIndex = id / 6u;
    const uint cornerIndex   = id % 6u;

    // 1粒につき乱数を5本引くので、番号を8刻みにして種の衝突を避ける
    const uint seed = (uint)fadeParams.z + particleIndex * 8u;

    //----------------------------------------------------------
    // ボリューム内のどこに居るか（0〜1をボリュームの大きさへ引き伸ばす）
    //----------------------------------------------------------
    float3 unitPos = float3(Rand01(seed + 0u), Rand01(seed + 1u), Rand01(seed + 2u));

    //----------------------------------------------------------
    // 層（0 = 大きく暗くゆっくり＝灰 ⇔ 1 = 小さく明るく速い＝火の粉）
    //
    // 乱数を1本だけ引いて3つのパラメータを同じ向きに補間するのが要点。
    // 別々の乱数にすると「大きくて速くて明るい粒」が混ざって画が汚れる。
    // 1本にまとめることで、1回の描画でも遠近の層があるように見える
    //----------------------------------------------------------
    float layer      = Rand01(seed + 3u);
    float radius     = lerp(sizeParams.y, sizeParams.x, layer);
    float brightness = lerp(sizeParams.z, sizeParams.w, layer);
    float speedScale = lerp(swayParams.y, swayParams.z, layer);

    // 揺らぎときらめきの位相。粒子ごとにずらして一斉に動くのを防ぐ
    float phase = Rand01(seed + 4u) * TWO_PI;

    const float time  = volumeParams.w;
    const float omega = driftParams.w;

    //----------------------------------------------------------
    // 一定のドリフトに、軸ごとに周期をずらしたサインの揺らぎを重ねる。
    // 周期を互いに割り切れない倍率でずらすと、軌道が閉じずに漂って見える
    //----------------------------------------------------------
    float3 drift = driftParams.xyz * (speedScale * time);
    float3 sway  = float3(sin(time * omega         + phase),
                          sin(time * omega * 0.73f + phase * 1.7f),
                          cos(time * omega * 0.61f + phase * 2.3f)) * swayParams.x;

    float3 rawPos = unitPos * volumeParams.xyz + drift + sway;

    //----------------------------------------------------------
    // カメラを中心にボリュームで折り返す（この技法の要）
    //
    // frac()は負数でも0〜1を返すので、これだけで座標がボリュームの
    // 剰余に畳み込まれる。カメラが動けば折り返しの基準も一緒に動くため、
    // 有限の粒子数で「どこまでも続く粒子場」になる
    //----------------------------------------------------------
    float3 halfSize = volumeParams.xyz * 0.5f;
    float3 origin   = cameraPos.xyz - halfSize;
    float3 worldPos = origin + frac((rawPos - origin) / volumeParams.xyz) * volumeParams.xyz;

    //----------------------------------------------------------
    // 境界フェード
    //
    // 折り返しでは座標が不連続に飛ぶので、飛ぶ瞬間に粒子が消えていないと
    // ポップとして見えてしまう。ボリュームの面までに必ず0になるよう、
    // 各軸の比率の最大値（＝立方体の面への近さ）で減衰させる
    //----------------------------------------------------------
    float3 edgeRatio = abs(worldPos - cameraPos.xyz) / max(halfSize, 1.0e-4f);
    float  edgeMax   = max(edgeRatio.x, max(edgeRatio.y, edgeRatio.z));
    float  edgeFade  = 1.0f - smoothstep(fadeParams.x, 1.0f, edgeMax);

    //----------------------------------------------------------
    // 近接フェード（目の前の1粒が画面を覆ってしまうのを防ぐ）
    //----------------------------------------------------------
    float viewDist = length(worldPos - cameraPos.xyz);
    float nearFade = smoothstep(0.0f, max(fadeParams.y, 1.0e-4f), viewDist);

    //----------------------------------------------------------
    // きらめき（揺らぎとは違う周期で明滅させて火の粉のちらつきを作る）
    //----------------------------------------------------------
    float twinkle = lerp(1.0f - swayParams.w, 1.0f,
                         0.5f + 0.5f * sin(time * omega * 1.9f + phase * 3.1f));

    //----------------------------------------------------------
    // ビルボード
    //
    // このエンジンは行ベクトル規約（mul(v, view)）なので、ビュー行列の
    // 「列」がそのままカメラの右・上ベクトルになる。
    // _m00/_m10/_m20 が0列目＝右、_m01/_m11/_m21 が1列目＝上
    //----------------------------------------------------------
    float3 camRight = float3(view._m00, view._m10, view._m20);
    float3 camUp    = float3(view._m01, view._m11, view._m21);

    float2 corner = kCorners[cornerIndex];
    float3 offset = (camRight * corner.x + camUp * corner.y) * radius;

    output.position = mul(float4(worldPos + offset, 1.0f), viewProj);
    output.local    = corner;

    // ブレンドは SRC_ALPHA / ONE の加算なので、色には事前乗算しない
    output.color = float4(colorParams.rgb * (colorParams.w * brightness * twinkle),
                          edgeFade * nearFade);

    return output;
}

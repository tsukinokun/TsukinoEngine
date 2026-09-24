//--------------------------------------------------------------
//! @file   Scene.hlsli
//! @brief  シーン定数バッファ(b0)の共通定義
//! @author 山﨑愛
//! @note   b0はフレーム内で全オブジェクト共通のデータ。VS・PSを問わず
//!         ほぼ全てのシェーダーが触るため、ここに一元化する
//!         （PBR.hlsliがBRDFを、Material.hlsliがb2を一元化しているのと同じ考え方）。
//!         b0しか要らないシェーダーがPBR.hlsliを引き込まずに済むよう、
//!         PBR.hlsliとは別ファイルにしてある。
//--------------------------------------------------------------
#ifndef TSUKINO_SCENE_HLSLI
#define TSUKINO_SCENE_HLSLI

//--------------------------------------------------------------
//! カスケードシャドウの枚数
//! @note ShadowPass::kCascadeCount と一致させること。
//!       cascadeViewProj の要素数がこれで決まるため、b0のレイアウトに直結する
//--------------------------------------------------------------
#define TSUKINO_SHADOW_CASCADE_COUNT 4

//--------------------------------------------------------------
//! シーン用定数バッファ (b0)
//! @note Tsukino/Renderer/ConstantBuffer.hpp の CBufferScene と
//!       1バイト単位で一致させること。
//!
//!       末尾のメンバは宣言を省略しても前方の配置が変わらないため、
//!       「使う所まで書けばよい」と考えたくなるが、それをやってはいけない。
//!       途中のメンバを1つ飛ばした瞬間に、それ以降が全てずれるためである。
//!
//!       実際にShadowMapStatic.vs.hlslがinvViewProjを書き忘れており、
//!       matrixは64バイトなのでlightViewProjが64バイト手前を指し、
//!       invViewProjの中身を読んでいた。コンパイルも通るし警告も出ず、
//!       スタティックメッシュの影だけが静かに壊れていた。
//!       宣言を手で書かず、このファイルをincludeすればその事故は起きない。
//!
//!       Sprite.vs.hlsl / Sprite3D.vs.hlsl / DebugLine.vs.hlslだけは
//!       独自宣言のまま残してある。読むのが先頭3行列だけで構造上ずれようがなく、
//!       メンバ名も View / Projection / ViewProj と大文字始まりの別物のため、
//!       揃えると呼び出し側の改名が発生するだけで安全性は増えない。
//!
//!       ゲーム側リポジトリのシェーダーからもそのままincludeできる。
//!       ShaderImporterが「インクルード元のディレクトリ → エンジンのShaders」の
//!       順で探すため、相対パスを書く必要は無い（ShaderIncludeHandler参照）。
//!
//!       なお、C++側のstatic_assertで守れるのはC++側の構造体だけである。
//!       このファイルのメンバを増減させてもアサートは全て通り、
//!       描画結果だけが静かに壊れる。片方を触ったら必ず両方を見ること
//--------------------------------------------------------------
cbuffer CBufferScene : register(b0)
{
    matrix view;                                            // 0
    matrix projection;                                      // 64
    matrix viewProj;                                        // 128
    matrix invViewProj;                                     // 192  viewProjの逆行列（スカイ・ポストエフェクト等で使用）
    matrix cascadeViewProj[TSUKINO_SHADOW_CASCADE_COUNT];   // 256  カスケードごとのライト空間ViewProjection（近→遠）
    float4 lightDir;                                        // 512  xyz: ライト方向（正規化済み）
    float4 lightColor;                                      // 528  xyz: ライトの色, w: 強度
    float4 cameraPos;                                       // 544  xyz: カメラのワールド座標, w: 未使用
    matrix prevViewProj;                                    // 560  前フレームのViewProjection行列（速度バッファ生成用）
    float4 timeParams;                                      // 624  x: 起動からの経過秒, y: 前フレームからの経過秒, z: sin(x), w: cos(x)
    float4 screenParams;                                    // 640  xy: 描画領域の解像度(px), zw: その逆数
    float4 shadowParams;                                    // 656  x: シャドウマップの一辺(px), y: その逆数, z: シャドウパスが描画中のカスケード番号, w: 1/奥行き
    float4 cascadeTexelWorld;                               // 672  xyzw: 各カスケードの1テクセルのワールド幅（シャドウバイアスの単位。カスケード4枚ぶん）
};

#endif    // TSUKINO_SCENE_HLSLI

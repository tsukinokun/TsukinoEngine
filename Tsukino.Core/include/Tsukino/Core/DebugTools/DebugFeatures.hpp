//-------------------------------------------------------------
//! @file   DebugFeatures.hpp
//! @brief  デバッグ機能ごとに個別に有効/無効を切り替えるためのマクロ定義
//-------------------------------------------------------------
#pragma once

// デバッグ機能ごとに個別のマクロで有効/無効を切り替える。
// _DEBUG (Debugビルド) の中でのみ選択可能 -> Releaseでは常に全て無効。
// 使わない機能は #define 行をコメントアウトすること。
#ifdef _DEBUG

// 物理コリジョン形状 / 接地判定ボックスのデバッグ描画 (F5キーでトグル)
#define TSUKINO_DEBUG_COLLISION_DRAW

// TSUKINO_DEBUG_COLLISION_DRAW が有効な場合に、起動直後からワイヤーフレーム描画を
// ONの状態にしておく（F5キーで押さなくても常に表示される。F5でOFFへの切り替えは引き続き可能）
//#define TSUKINO_DEBUG_COLLISION_DRAW_ALWAYS_ON

#endif    // _DEBUG

//-------------------------------------------------------------
// ここから下は _DEBUG の外。Release ビルドでも有効にできる。
//
// 性能計測は Release で行う必要がある。Debug は optimize "Off" かつ
// ENTT_ASSERT が生きているため、CPU時間が実態と桁で乖離してしまい、
// 「どのシステムが重いのか」の判断材料にならないため。
// Release も symbols "On" なので、外部プロファイラとの併用もできる。
//-------------------------------------------------------------

// フレームプロファイラ（システム別CPU時間・描画統計・VSyncトグル）。
// 無効にすると計測コードは全て空実装へコンパイルされ、実行時コストはゼロになる。
#define TSUKINO_ENABLE_FRAME_PROFILER

// 敵の大量スポーンによる負荷試験（ホットキー操作・HUD表示）。
// TSUKINO_ENABLE_FRAME_PROFILER が無効だと計測値が出ないため、通常は両方を有効にする。
#define TSUKINO_ENABLE_STRESS_TEST

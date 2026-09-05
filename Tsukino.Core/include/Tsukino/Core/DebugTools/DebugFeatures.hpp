//-------------------------------------------------------------
//! @file   DebugFeatures.hpp
//! @brief  デバッグ機能ごとに個別に有効/無効を切り替えるためのマクロ定義
//-------------------------------------------------------------
#pragma once

// デバッグ機能ごとに個別のマクロで有効/無効を切り替える。
// _DEBUG (Debugビルド) の中でのみ選択可能 -> Releaseでは常に全て無効。
//
// このヘッダは取り込み側の翻訳単位にも届く公開ヘッダなので、無条件に #define しない。
// 既定は有効のまま、取り込み側が TSUKINO_DISABLE_* を定義すれば
// エンジンのソースを書き換えずに機能を落とせる。
#ifdef _DEBUG

// 物理コリジョン形状 / 接地判定ボックスのデバッグ描画 (F5キーでトグル)
#ifndef TSUKINO_DISABLE_DEBUG_COLLISION_DRAW
#    ifndef TSUKINO_DEBUG_COLLISION_DRAW
#        define TSUKINO_DEBUG_COLLISION_DRAW
#    endif
#endif

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
// 製品ビルドで落としたい場合は TSUKINO_DISABLE_FRAME_PROFILER を定義する。
#ifndef TSUKINO_DISABLE_FRAME_PROFILER
#    ifndef TSUKINO_ENABLE_FRAME_PROFILER
#        define TSUKINO_ENABLE_FRAME_PROFILER
#    endif
#endif

// 敵の大量スポーンによる負荷試験（ホットキー操作・HUD表示）。
// TSUKINO_ENABLE_FRAME_PROFILER が無効だと計測値が出ないため、通常は両方を有効にする。
// 製品ビルドで落としたい場合は TSUKINO_DISABLE_STRESS_TEST を定義する。
#ifndef TSUKINO_DISABLE_STRESS_TEST
#    ifndef TSUKINO_ENABLE_STRESS_TEST
#        define TSUKINO_ENABLE_STRESS_TEST
#    endif
#endif

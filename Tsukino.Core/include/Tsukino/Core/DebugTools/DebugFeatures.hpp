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

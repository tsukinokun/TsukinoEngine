//------------------------------------------------------------
//! @file	pch.h
//! @brief  プリコンパイル済みヘッダーファイル
//! @note   Tsukino.Rendererプロジェクトのビルド時間を短縮用
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
#define NOMINMAX
// Windows API
#include <Tsukino/Core/WindowsLean.hpp>    // Windows のときだけ windows.h を取り込む

// STL（Renderer が頻繁に使うものだけ）
#include <memory>    // スマートポインタを使用するための標準ライブラリ
#include <vector>    // 動的配列を使用するための標準ライブラリ
#include <string>    // 文字列操作のための標準ライブラリ
#include <array>     // 固定サイズの配列を使用するための標準ライブラリ

//------------------------------------------------------------
//! @file	pch.h
//! @brief  プリコンパイル済みヘッダーファイル
//! @note   Tsukino.Coreプロジェクトのビルド時間を短縮用
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
// windows.h は入れない（Tsukino.Core はどの環境でもビルドできるようにする。
// Windows だけの処理は、そのファイルで WindowsLean.hpp を _WIN32 の中で取り込む）
#include <string>        // 文字列操作のための標準ライブラリ
#include <vector>        // 動的配列を使用するための標準ライブラリ
#include <filesystem>    // ファイルシステム操作のための標準ライブラリ

// hlslpp
#include <hlsl++.h>

// entt
#include <entt/entt.hpp>

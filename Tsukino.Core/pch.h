//------------------------------------------------------------
//! @file	pch.h
//! @brief  プリコンパイル済みヘッダーファイル
//! @note   Tsukino.Coreプロジェクトのビルド時間を短縮用
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
#define NOMINMAX
#include <Windows.h>     // Windows APIの基本的な機能を提供するヘッダーファイル
#include <string>        // 文字列操作のための標準ライブラリ
#include <vector>        // 動的配列を使用するための標準ライブラリ
#include <filesystem>    // ファイルシステム操作のための標準ライブラリ

// hlslpp
#include <hlsl++.h>

// entt
#include <entt/entt.hpp>

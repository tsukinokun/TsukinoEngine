//------------------------------------------------------------
//! @file	pch.h
//! @brief  プリコンパイル済みヘッダーファイル
//! @note   Tsukino.Sandboxプロジェクトのビルド時間を短縮用
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
#define NOMINMAX        // Windows APIのminとmaxマクロを無効化するための定義
#include <Tsukino/Core/WindowsLean.hpp>    // Windows のときだけ windows.h を取り込む
#include <cstdint>      // 固定幅整数型を使用するための標準ライブラリ
#include <string>       // 文字列操作のための標準ライブラリ
#include <unordered_map>
#include <vector>
#include <memory>       // スマートポインタを使用するための標準ライブラリ

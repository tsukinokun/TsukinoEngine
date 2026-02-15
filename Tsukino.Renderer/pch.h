//------------------------------------------------------------
//! @file	pch.h
//! @brief  プリコンパイル済みヘッダーファイル
//! @note   Tsukino.Rendererプロジェクトのビルド時間を短縮用
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once

// Windows API
#include <Windows.h>    // Windows APIの基本的な機能を提供するヘッダーファイル

// DirectX 11
#include <d3d11.h>          // Direct3D 11の基本的な機能を提供するヘッダーファイル
#include <dxgi.h>           // DirectX Graphics Infrastructureの基本的な機能を提供するヘッダーファイル
#include <d3dcompiler.h>    // HLSLシェーダーのコンパイルに必要なヘッダーファイル

// WRL (ComPtr)
#include <wrl.h>    // Microsoft::WRL::ComPtrを使用するためのヘッダーファイル

// STL（Renderer が頻繁に使うものだけ）
#include <memory>    // スマートポインタを使用するための標準ライブラリ
#include <vector>    // 動的配列を使用するための標準ライブラリ
#include <string>    // 文字列操作のための標準ライブラリ

//----------------------------------------------------------------------------
//! @file   NativeHandles.hpp
//! @brief  OS と描画 API の生のハンドルを、型を伏せて受け渡す構造体
//! @detail 公開ヘッダに windows.h や d3d11.h を持ち込まずに、ウィンドウや描画デバイスを
//!         やり取りするためのものです。中身の型は環境と描画 API によって変わります。
//----------------------------------------------------------------------------
#pragma once

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! 描画先のウィンドウです。
    //! @note  handle は Windows では HWND。Window::GetNativeHandle() の値をそのまま入れる
    //------------------------------------------------------------------------
    struct NativeWindow {
        void* handle = nullptr;    // OS のウィンドウハンドル
    };

    //------------------------------------------------------------------------
    //! 描画 API の生のデバイスです。
    //! @note  Renderer の外で描画 API を直接使う部品（Effekseer など）のためのもの。
    //!        DX11 では device が ID3D11Device*、context が ID3D11DeviceContext*
    //------------------------------------------------------------------------
    struct NativeGraphicsContext {
        void* device  = nullptr;    // 描画デバイス
        void* context = nullptr;    // デバイスコンテキスト（描画命令の発行先）
    };

}    // namespace Tsukino::Renderer

//--------------------------------------------------------------
//! @file	main.cpp
//! @brief	Sandbox(DLL)のエントリポイント
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Windows.h>
#include "Tsukino/Core/Window.h"
#include "Tsukino/Renderer/Renderer.h"
//--------------------------------------------------------------
// アプリケーションのエントリポイント
//! @param hInstance アプリケーションインスタンス
//! @param hPrevInstance 非推奨（常にNULL）
//! @param lpCmdLine コマンドライン引数
//! @param nCmdShow ウィンドウ表示状態（例：SW_SHOW）
//! @return 終了コード（通常は0）
//--------------------------------------------------------------
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // ウィンドウ生成
    Tsukino::Core::Window window;
    if(!window.Create("TsukinoEngine", 1280, 720)) {
        return -1;
    }

    // レンダラー生成
    Tsukino::Renderer::Renderer renderer;
    if(!renderer.Initialize(window.GetHWND(), window.GetWidth(), window.GetHeight())) {
        return -1;
    }

    // メインループ
    while(window.ProcessMessages()) {
        renderer.Render();
    }

    // 終了処理
    renderer.Destroy();

    // ウィンドウは自動的に破棄される
    return 0;
}

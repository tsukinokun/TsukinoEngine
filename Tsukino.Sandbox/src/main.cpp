//--------------------------------------------------------------
//! @file	main.cpp
//! @brief	Sandbox(DLL)のエントリポイント
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Windows.h>
//--------------------------------------------------------------
// アプリケーションのエントリポイント
//! @param hInstance アプリケーションインスタンス
//! @param hPrevInstance 非推奨（常にNULL）
//! @param lpCmdLine コマンドライン引数
//! @param nCmdShow ウィンドウ表示状態（例：SW_SHOW）
//! @return 終了コード（通常は0）
//--------------------------------------------------------------
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	//Tsukino::Core::Window window;
	//window.Create("TsukinoEngine", 1280, 720);

	//Tsukino::Renderer::Renderer::Initialize(window.GetHWND());

	//while (window.ProcessMessages())
	//{
	//	Tsukino::Renderer::Renderer::BeginFrame();
	//	Tsukino::Renderer::Renderer::EndFrame();
	//}

	//Tsukino::Renderer::Renderer::Shutdown();
	return 0;
}

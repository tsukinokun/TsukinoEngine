//--------------------------------------------------------------
//! @file	WinMain.cpp
//! @brief	Sandboxのエントリポイント
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Tsukino.EngineIntegration/EngineAPI.hpp>
#include <Tsukino/Tsukino.EngineIntegration/EngineIntegration.hpp>
//#include <Tsukino/Renderer/Renderer.hpp>
//#include <Tsukino/Renderer/DX11/PipelineFactory.hpp>
//#include <Tsukino/Renderer/DX11/PipelineState.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
//#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
//#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>
//#include <Tsukino/Core/Path.hpp>
//#include <Tsukino/Core/Window.hpp>
#include <Tsukino/Core/Log.hpp>

#include <Windows.h>
//--------------------------------------------------------------
// アプリケーションのエントリポイント
//! @param hInstance アプリケーションインスタンス
//! @param hPrevInstance 非推奨（常にNULL）
//! @param lpCmdLine コマンドライン引数
//! @param nCmdShow ウィンドウ表示状態（例：SW_SHOW）
//! @return 終了コード（通常は0）
//--------------------------------------------------------------
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // ログの初期化
    Tsukino::EngineIntegration::EngineIntegration engineIntegration;
    // 初期化
    if(!engineIntegration.Initialize()) {
        // 初期化に失敗した場合はエラーログを出力して終了
        Tsukino::Core::Log::Error("Failed to initialize EngineIntegration.");
        return false;
    }

    Tsukino::EngineIntegration::EngineContext engineContext = engineIntegration.GetContext();
    Tsukino::EngineIntegration::EngineAPI     engineAPI(engineContext);

    //--------------------------------------------------------------
    // メインループ
    //--------------------------------------------------------------
    //while(window.ProcessMessages()) {
    //    renderer.Render();
    //}

    //--------------------------------------------------------------
    // AssetManager 終了処理
    //--------------------------------------------------------------
    Tsukino::Asset::AssetManager::Destroy();

    //--------------------------------------------------------------
    // ウィンドウは自動的に破棄される
    //--------------------------------------------------------------
    return 0;
}

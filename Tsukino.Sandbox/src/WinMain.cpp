//--------------------------------------------------------------
//! @file	WinMain.cpp
//! @brief	Sandboxのエントリポイント
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Window.hpp>
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

    //--------------------------------------------------------------
    // COM初期化
    //--------------------------------------------------------------
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    //--------------------------------------------------------------
    // AssetManager 初期化
    //--------------------------------------------------------------
    Tsukino::Asset::AssetManager::Initialize();

    //--------------------------------------------------------------
    // テスト用シェーダーをインポート
    //--------------------------------------------------------------
    {
        Tsukino::Core::Path path("Assets/Shaders/Triangle.ps.hlsl");    //パスオブジェクトを作成
        Tsukino::Asset::AssetHandle handle = Tsukino::Asset::AssetManager::Load(path);    // シェーダーアセットをロード

                // ロードに失敗した場合はエラーメッセージを表示
        if(!Tsukino::Asset::AssetManager::Exists(handle)) {
            MessageBoxA(nullptr, "Failed to import shader.", "Error", MB_OK);
        } else {
            Tsukino::Core::Ref<Tsukino::Asset::ShaderAsset> asset =
                std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(Tsukino::Asset::AssetManager::Get(handle));
            OutputDebugStringA("=== Shader Loaded ===\n");
            OutputDebugStringA(asset->source.c_str());
            OutputDebugStringA("\n=====================\n");
        }
    }

    //--------------------------------------------------------------
    // テストテクスチャをロード
    //--------------------------------------------------------------
    {
        Tsukino::Core::Path         path("Assets/Textures/test.jpg");    //パスオブジェクトを作成
        Tsukino::Asset::AssetHandle tex_handle = Tsukino::Asset::AssetManager::Load(path);
    }

    //--------------------------------------------------------------
    // ウィンドウ生成
    //--------------------------------------------------------------
    Tsukino::Core::Window window;
    if(!window.Create("TsukinoEngine", 1280, 720)) {
        return -1;
    }

    //--------------------------------------------------------------
    // レンダラー生成
    //--------------------------------------------------------------
    Tsukino::Renderer::Renderer renderer;
    if(!renderer.Initialize(window.GetHWND(), window.GetWidth(), window.GetHeight())) {
        return -1;
    }

    //--------------------------------------------------------------
    // メインループ
    //--------------------------------------------------------------
    while(window.ProcessMessages()) {
        renderer.Render();
    }

    //--------------------------------------------------------------
    // AssetManager 終了処理
    //--------------------------------------------------------------
    Tsukino::Asset::AssetManager::Destroy();

    //--------------------------------------------------------------
    // ウィンドウは自動的に破棄される
    //--------------------------------------------------------------
    return 0;
}

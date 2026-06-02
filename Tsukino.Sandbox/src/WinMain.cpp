//--------------------------------------------------------------
//! @file	WinMain.cpp
//! @brief	Sandboxのエントリポイント
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineIntegration.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Sandbox/Scene/SampleScene1.hpp>
#include <Tsukino/Sandbox/Scene/JumpGameSampleScene.hpp>
#include <Tsukino/Sandbox/Scene/BlockBreakingSampleScene.hpp>

#include <Windows.h>
#include <memory>

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
    if(!engineIntegration.Initialize(1700, 1000)) {
        // 初期化に失敗した場合はエラーログを出力して終了
        Tsukino::Core::Log::Error("Failed to initialize EngineIntegration.");
        return false;
    }

    Tsukino::EngineIntegration::EngineContext& engineContext = engineIntegration.GetContext();
    Tsukino::EngineIntegration::EngineAPI      engineAPI(engineContext);

    //--------------------------------------------------------------
    // 最初のシーンを登録・開始
    //--------------------------------------------------------------
    //engineAPI.ChangeScene(std::make_unique<Tsukino::Sandbox::SampleScene1>());
    engineAPI.ChangeScene(std::make_unique<Tsukino::Sandbox::JumpGameSampleScene>());
    //engineAPI.ChangeScene(std::make_unique<Tsukino::Sandbox::BlockBreakingSampleScene>());

    //--------------------------------------------------------------
    // メインループ
    //--------------------------------------------------------------
    // テスト用の固定デルタタイム
    const float deltaTime = 1.0f / 60.0f;

    //--------------------------------------------------------------
    // メインループ
    //--------------------------------------------------------------
    while(engineAPI.ProcessMessages()) {
        // 一括更新
        engineAPI.Update(deltaTime);
        // 描画処理
        engineAPI.Render();
    }

    //--------------------------------------------------------------
    // ウィンドウは自動的に破棄される
    //--------------------------------------------------------------
    return 0;
}

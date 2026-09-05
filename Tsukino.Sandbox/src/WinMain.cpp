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
#include <Tsukino/Sandbox/Scene/WaterGameSampleScene.hpp>
#include <Tsukino/Sandbox/Scene/DeferredLightSampleScene.hpp>

#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>

namespace {
    //--------------------------------------------------------------
    //! @brief  コマンドライン引数から起動するサンプルシーンを選ぶ
    //! @param  commandLine [in] WinMain が受け取るコマンドライン文字列
    //! @return 生成したシーン
    //! @note   以前は ChangeScene の呼び出しをコメントアウトで切り替えていたが、
    //!         サンプルを見るたびに再ビルドが要るうえ、どれが既定なのかも
    //!         ソースを読まないと分からなかったため引数で選べるようにした。
    //--------------------------------------------------------------
    std::unique_ptr<Tsukino::EngineIntegration::GameSceneBase> CreateSceneFromCommandLine(const char* commandLine) {
        const std::string argument = (commandLine != nullptr) ? commandLine : "";

        if(argument.find("blocks") != std::string::npos) {
            return std::make_unique<Tsukino::Sandbox::BlockBreakingSampleScene>();
        }
        if(argument.find("water") != std::string::npos) {
            return std::make_unique<Tsukino::Sandbox::WaterGameSampleScene>();
        }
        // ディファードレンダリングの多光源ショーケース（F1でライト数を 0/1/16/64 に切り替え）
        if(argument.find("lights") != std::string::npos) {
            return std::make_unique<Tsukino::Sandbox::DeferredLightSampleScene>();
        }
        // エンジンAPIの動作確認シーン。PrefabFactory のテストを実行してログへ結果を出す
        if(argument.find("api") != std::string::npos) {
            return std::make_unique<Tsukino::Sandbox::SampleScene1>();
        }

        // 既定は「起動してすぐ遊べる」ものにする
        return std::make_unique<Tsukino::Sandbox::JumpGameSampleScene>();
    }
}    // namespace

//--------------------------------------------------------------
// アプリケーションのエントリポイント
//! @param hInstance アプリケーションインスタンス
//! @param hPrevInstance 非推奨（常にNULL）
//! @param lpCmdLine コマンドライン引数
//! @param nCmdShow ウィンドウ表示状態（例：SW_SHOW）
//! @return 終了コード（通常は0）
//--------------------------------------------------------------
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR lpCmdLine, _In_ int) {
    // DPIスケーリングの無効化
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    //--------------------------------------------------------------
    // ログをファイルへ出す。
    // これを呼ばないとLogはOutputDebugStringAにしか出ず、デバッガの外で
    // 実行したときは「Prefab file not found」のような致命的な警告も、
    // 落ちた位置も一切見えない。最初に動かすサンプルなので既定で有効にする
    //--------------------------------------------------------------
    Tsukino::Core::Log::SetLogFile("Logs/Tsukino.log");

    Tsukino::EngineIntegration::EngineIntegration engineIntegration;
    // 初期化
    if(!engineIntegration.Initialize(1700, 1000, "TsukinoEngine")) {
        // 初期化に失敗した場合はエラーログを出力して終了
        // (以前は return false と書かれており、失敗なのに終了コード 0＝成功を返していた)
        Tsukino::Core::Log::Error("Failed to initialize EngineIntegration.");
        return EXIT_FAILURE;
    }

    Tsukino::EngineIntegration::EngineContext& engineContext = engineIntegration.GetContext();
    Tsukino::EngineIntegration::EngineAPI      engineAPI(engineContext);

    //--------------------------------------------------------------
    // 最初のシーンを登録・開始
    //
    //   Tsukino.Sandbox.exe            ジャンプゲーム（既定）
    //   Tsukino.Sandbox.exe blocks     ブロック崩し
    //   Tsukino.Sandbox.exe water      水面のミニゲーム
    //   Tsukino.Sandbox.exe lights     多光源ショーケース
    //   Tsukino.Sandbox.exe api        エンジンAPIの動作確認シーン
    //--------------------------------------------------------------
    engineAPI.ChangeScene(CreateSceneFromCommandLine(lpCmdLine));

    //--------------------------------------------------------------
    // メインループ
    // deltaTimeは実測の経過時間を使う。固定値（1/60）だとPresent(1,0)のVSyncが
    // 60Hzより高いリフレッシュレートのディスプレイに同期した場合、実際のフレーム間隔より
    // 大きい値としてシミュレーションが進んでしまい、アニメーション等が実際より速く再生される
    // （攻撃モーションが暴れて見える不具合の原因だった）
    //--------------------------------------------------------------
    auto lastTime = std::chrono::steady_clock::now();

    while(engineAPI.ProcessMessages()) {
        auto  currentTime = std::chrono::steady_clock::now();
        float deltaTime    = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime            = currentTime;
        // ウィンドウドラッグ等で1フレームが極端に長くなった場合の暴走を防ぐ上限
        deltaTime = std::min(deltaTime, 1.0f / 15.0f);

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

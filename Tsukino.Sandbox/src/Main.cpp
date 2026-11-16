//--------------------------------------------------------------
//! @file	Main.cpp
//! @brief	Sandboxのエントリポイント（どの OS でも SDL の main から始まる）
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineIntegration.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Sandbox/Scene/SampleScene1.hpp>
#include <Tsukino/Sandbox/Scene/DeferredLightSampleScene.hpp>

// SDL_main.h は main を取り込むファイルで1回だけ取り込む（Windows では WinMain、
// iOS・Android ではそれぞれの起動の仕組みから main を呼ぶようにしてくれる）
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>

namespace {
    //--------------------------------------------------------------
    //! @brief  コマンドライン引数から起動するサンプルシーンを選ぶ
    //! @param  commandLine [in] 起動引数（空白でつないだもの）
    //! @return 生成したシーン
    //! @note   以前は ChangeScene の呼び出しをコメントアウトで切り替えていたが、
    //!         サンプルを見るたびに再ビルドが要るうえ、どれが既定なのかも
    //!         ソースを読まないと分からなかったため引数で選べるようにした。
    //--------------------------------------------------------------
    std::unique_ptr<Tsukino::EngineIntegration::GameSceneBase> CreateSceneFromCommandLine(const char* commandLine) {
        const std::string argument = (commandLine != nullptr) ? commandLine : "";

        // エンジンAPIの動作確認シーン。PrefabFactory のテストを実行してログへ結果を出す
        if(argument.find("api") != std::string::npos) {
            return std::make_unique<Tsukino::Sandbox::SampleScene1>();
        }

        // 既定（"lights" 指定時も）はディファードレンダリングの多光源ショーケース
        // （F1でライト数を 0/1/16/64 に切り替え）
        return std::make_unique<Tsukino::Sandbox::DeferredLightSampleScene>();
    }
}    // namespace

//--------------------------------------------------------------
// アプリケーションのエントリポイント
//! @param argc 引数の数
//! @param argv 引数（UTF-8。argv[0] は実行ファイル）
//! @return 終了コード（通常は0）
//! @note   画面の拡大率（DPI）への対応は SDL が行う
//--------------------------------------------------------------
int main(int argc, char* argv[]) {
    std::string commandLine;
    for(int i = 1; i < argc; ++i) {
        if(!commandLine.empty())
            commandLine += ' ';
        commandLine += argv[i];
    }
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
    //   Tsukino.Sandbox.exe            多光源ショーケース（既定。lights でも可）
    //   Tsukino.Sandbox.exe api        エンジンAPIの動作確認シーン
    //--------------------------------------------------------------
    engineAPI.ChangeScene(CreateSceneFromCommandLine(commandLine.c_str()));

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
        float deltaTime   = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime          = currentTime;
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

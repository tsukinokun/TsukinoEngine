//--------------------------------------------------------------
//! @file   Log.cpp
//! @brief  ログ出力クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Core/Log.hpp>
#include <windows.h>

#include <ctime>
#include <filesystem>
#include <fstream>
// 名前空間 : Tsukino::Core
namespace Tsukino::Core {

    // 出力先のログファイル。空の間はファイルへ書き出さない
    std::string Log::s_LogFilePath;

    //--------------------------------------------------------------
    //! @brief 情報ログ出力
    //--------------------------------------------------------------
    void Log::Info(const std::string& msg) {
        Write("Info", msg);
    }

    //--------------------------------------------------------------
    //! @brief 警告ログ出力
    //--------------------------------------------------------------
    void Log::Warn(const std::string& msg) {
        Write("Warn", msg);
    }

    //--------------------------------------------------------------
    //! @brief エラーログ出力
    //--------------------------------------------------------------
    void Log::Error(const std::string& msg) {
        Write("Error", msg);
    }

    //--------------------------------------------------------------
    //! ログの出力先ファイルを設定します。
    //--------------------------------------------------------------
    void Log::SetLogFile(const std::string& filePath) {
        std::filesystem::path outputPath(filePath);

        // 親ディレクトリが無ければ作成する。
        // ここで失敗してもログが出なくなるだけなので、実行そのものは続行する
        if (outputPath.has_parent_path()) {
            std::error_code errorCode;
            std::filesystem::create_directories(outputPath.parent_path(), errorCode);
        }

        s_LogFilePath = filePath;

        // 追記運用のため、起動ごとの区切りを1行入れておく。
        // これが無いと前回の実行分と混ざって、どこからが今回の出力か分からなくなる
        std::ofstream stream(s_LogFilePath, std::ios::app);
        if (!stream)
            return;

        std::time_t now = std::time(nullptr);
        std::tm     localTime{};
        localtime_s(&localTime, &now);

        char timestamp[32]{};
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &localTime);

        stream << "---------------- log started " << timestamp << " ----------------\n";
    }

    //--------------------------------------------------------------
    //! ファイルへのログ出力を停止します。
    //--------------------------------------------------------------
    void Log::CloseLogFile() {
        s_LogFilePath.clear();
    }

    //--------------------------------------------------------------
    //! レベル表記を付けた1行を、デバッグ出力とログファイルへ書き出します。
    //--------------------------------------------------------------
    void Log::Write(const char* level, const std::string& msg) {
        std::string out = std::string("[") + level + "] " + msg + "\n";
        OutputDebugStringA(out.c_str());

        if (s_LogFilePath.empty())
            return;

        // 都度開いて追記する。ログの頻度は低く、それより
        // クラッシュしてもそこまでの行が確実に残ることを優先する
        std::ofstream stream(s_LogFilePath, std::ios::app);
        if (stream) {
            stream << out;
        }
    }

}    // namespace Tsukino::Core

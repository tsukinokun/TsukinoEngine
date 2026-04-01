//--------------------------------------------------------------
//! @file   FontImporter.cpp
//! @brief  フォントのインポータークラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Font/FontImporter.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>

#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief  外部プロセスを実行して終了コードを返す
    //--------------------------------------------------------------
    bool FontImporter::RunProcess(const Tsukino::Core::Path& executablePath, const std::wstring& arguments, const Tsukino::Core::Path& workingDir) {
        //--------------------------------------------------------------
        // 実行パスと引数を結合してコマンドラインを作成
        //--------------------------------------------------------------
        std::wstring         commandLine = L"\"" + executablePath.ToWString() + L"\" " + arguments;
        std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
        mutableCommandLine.push_back(L'\0');

        STARTUPINFOW        startupInfo{};
        PROCESS_INFORMATION processInfo{};
        startupInfo.cb = sizeof(STARTUPINFOW);

        const BOOL created = ::CreateProcessW(
            nullptr, mutableCommandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, workingDir.ToWString().c_str(), &startupInfo, &processInfo);

        if(!created) {
            Tsukino::Core::Log::Error("Failed to launch MakeSpriteFont.exe. error=" + std::to_string(::GetLastError()));
            return false;
        }

        ::WaitForSingleObject(processInfo.hProcess, INFINITE);

        DWORD exitCode = 1;
        ::GetExitCodeProcess(processInfo.hProcess, &exitCode);

        ::CloseHandle(processInfo.hThread);
        ::CloseHandle(processInfo.hProcess);

        if(exitCode != 0) {
            Tsukino::Core::Log::Error("MakeSpriteFont.exe failed. exitCode=" + std::to_string(exitCode));
            return false;
        }

        return true;
    }

    //--------------------------------------------------------------
    //! @brief  フォントのインポート関数
    //--------------------------------------------------------------
    bool FontImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        //--------------------------------------------------------------
        // ルートからの絶対パスで開く
        //--------------------------------------------------------------
        Tsukino::Core::Path baseDir           = Tsukino::IO::FileSystem::GetAssetRootPath();
        Tsukino::Core::Path absoluteInputPath = baseDir / inputPath;

        //--------------------------------------------------------------
        // outputDirectory(Cache/) に inputPath(相対) を結合して階層を維持
        //--------------------------------------------------------------
        Tsukino::Core::Path outputPath = outputDirectory / inputPath;
        outputPath.replace_extension(".spritefont");

        //--------------------------------------------------------------
        // 親ディレクトリの生成
        //--------------------------------------------------------------
        Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path());

        //--------------------------------------------------------------
        // 入力拡張子チェック
        //--------------------------------------------------------------
        std::string ext = inputPath.extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if(ext != ".ttf" && ext != ".otf" && ext != ".ttc") {
            Tsukino::Core::Log::Error("Unsupported font source format: " + inputPath.string());
            return false;
        }

        //--------------------------------------------------------------
        // MakeSpriteFont.exe の存在確認
        //--------------------------------------------------------------
        Tsukino::Core::Path toolPath = baseDir / "Tools/MakeSpriteFont.exe";
        if(!Tsukino::IO::FileSystem::Exists(toolPath)) {
            Tsukino::Core::Log::Error("MakeSpriteFont.exe not found: " + toolPath.string());
            return false;
        }

        //--------------------------------------------------------------
        // ttf/otf から .spritefont 変換
        // 引数形式: MakeSpriteFont.exe "<input>" "<output>"
        //--------------------------------------------------------------
        const std::wstring arguments = L"\"" + absoluteInputPath.ToWString() + L"\" \"" + outputPath.ToWString() + L"\"";
        if(!RunProcess(toolPath, arguments, baseDir)) {
            Tsukino::Core::Log::Error("Failed to convert font: " + absoluteInputPath.string() + " -> " + outputPath.string());
            return false;
        }

        Tsukino::Core::Log::Info("Font imported: " + outputPath.string());
        return true;
    }

}    // namespace Tsukino::Asset

//--------------------------------------------------------------
//! @file   AudioImporter.cpp
//! @brief  オーディオのインポータークラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Audio/AudioImporter.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>

#include <windows.h>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {

    //--------------------------------------------------------------
    //! @brief  外部プロセスを実行して終了コードを返す
    //--------------------------------------------------------------
    bool AudioImporter::RunProcess(const Tsukino::Core::Path& executablePath, const std::wstring& arguments, const Tsukino::Core::Path& workingDir) {   
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
            Tsukino::Core::Log::Error("Failed to launch XWBTool.exe. error=" + std::to_string(::GetLastError()));
            return false;
        }

        ::WaitForSingleObject(processInfo.hProcess, INFINITE);

        DWORD exitCode = 1;
        ::GetExitCodeProcess(processInfo.hProcess, &exitCode);

        ::CloseHandle(processInfo.hThread);
        ::CloseHandle(processInfo.hProcess);

        if(exitCode != 0) {
            Tsukino::Core::Log::Error("XWBTool.exe failed. exitCode=" + std::to_string(exitCode));
            return false;
        }

        return true;
    }

    //--------------------------------------------------------------
    //! @brief  フォントのインポート関数
    //--------------------------------------------------------------
    bool AudioImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        //--------------------------------------------------------------
        // パス内のフラグメント（#以降）を除外
        //--------------------------------------------------------------
        std::string rawPath       = inputPath.string();
        size_t      fragmentPos   = rawPath.find('#');
        std::string basePathValue = (fragmentPos == std::string::npos) ? rawPath : rawPath.substr(0, fragmentPos);

        Tsukino::Core::Path baseInputPath(basePathValue);

        //--------------------------------------------------------------
        // 拡張子チェック（フラグメント除外後）
        //--------------------------------------------------------------
        std::string ext = baseInputPath.extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if(ext != ".wav")
            return false;

        //--------------------------------------------------------------
        // 絶対入力パスの取得
        //--------------------------------------------------------------
        Tsukino::Core::Path baseDir           = Tsukino::IO::FileSystem::GetAssetRootPath();
        Tsukino::Core::Path absoluteInputPath = baseDir / baseInputPath;

        //--------------------------------------------------------------
        // 出力パスの決定（.xwb）
        // (baseInputPathがエンジン組み込みアセット由来の絶対パスの場合、そのまま
        //  outputDirectory / baseInputPath とすると絶対パスへ丸ごと置き換わってしまい、
        //  エンジンのソースツリー内に.xwbを書き込んでしまう。
        //  ToEngineRelativePath()で相対パスに戻してから結合する)
        //--------------------------------------------------------------
        Tsukino::Core::Path outputPath = outputDirectory / Tsukino::IO::FileSystem::ToEngineRelativePath(baseInputPath);
        outputPath.replace_extension(".xwb");

        //--------------------------------------------------------------
        // 親ディレクトリ作成
        //--------------------------------------------------------------
        // 出力先を作れないまま書き込みへ進むと、失敗が原因から遠い場所で
        // 「キャッシュが無い」として現れるため、ここで止める
        if(!Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path())) {
            Tsukino::Core::Log::Error("AudioImporter: Failed to create the output directory: "
                                      + outputPath.parent_path().string());
            return false;
        }

        //--------------------------------------------------------------
        // XWBTool.exe のパス
        // (エンジン自身が所有するツールのため、取り込み側リポジトリの
        //  GetAssetRootPath()ではなくGetEngineAssetRootPath()から解決する)
        //--------------------------------------------------------------
        Tsukino::Core::Path toolPath = Tsukino::IO::FileSystem::GetEngineAssetRootPath() / "Tools/XWBTool.exe";

        //--------------------------------------------------------------
        // 引数を作成して変換実行
        //--------------------------------------------------------------
        std::wstring arguments  = L"-nologo -y -nc -o ";
        arguments              += L"\"" + outputPath.ToWString() + L"\" ";
        arguments              += L"\"" + absoluteInputPath.ToWString() + L"\"";

        if(!RunProcess(toolPath, arguments, baseDir)) {
            Tsukino::Core::Log::Error("Failed to convert audio: " + absoluteInputPath.string() + " -> " + outputPath.string());
            return false;
        }

        Tsukino::Core::Log::Info("Audio imported: " + outputPath.string());
        return true;
    }

}    // namespace Tsukino::Asset

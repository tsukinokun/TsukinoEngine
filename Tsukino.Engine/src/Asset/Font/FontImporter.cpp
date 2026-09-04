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
#include <map>
#include <fstream>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief  文字列の前後の空白をトリムする関数
    //--------------------------------------------------------------
    std::wstring Trim(const std::wstring& s) {
        auto start = s.find_first_not_of(L" \t\r\n");
        auto end   = s.find_last_not_of(L" \t\r\n");
        return (start == std::wstring::npos) ? L"" : s.substr(start, end - start + 1);
    }

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
        // 絶対パスを取得
        //--------------------------------------------------------------
        Tsukino::Core::Path baseDir           = Tsukino::IO::FileSystem::GetAssetRootPath();
        Tsukino::Core::Path absoluteInputPath = baseDir / inputPath;

        //--------------------------------------------------------------
        // 拡張子チェック
        //--------------------------------------------------------------
        std::string ext = inputPath.extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if(ext != ".font")
            return false;

        //--------------------------------------------------------------
        // デフォルト設定の準備
        //--------------------------------------------------------------
        std::map<std::wstring, std::wstring> settings;
        settings[L"FaceName"]        = L"Arial";
        settings[L"Size"]            = L"24";
        settings[L"Italic"]          = L"false";
        settings[L"CharacterRegion"] = L"0x20-0x7E";    // デフォルトは英数字

        //--------------------------------------------------------------
        // .font ファイルの解析
        //--------------------------------------------------------------
        std::wifstream file(absoluteInputPath.ToWString());
        if(!file.is_open()) {
            Tsukino::Core::Log::Error("Failed to open .font file: " + absoluteInputPath.string());
            return false;
        }

        std::wstring line;
        while(std::getline(file, line)) {
            if(line.empty() || line[0] == L'#')
                continue;    // 空行とコメントをスキップ

            auto pos = line.find(L'=');
            if(pos != std::wstring::npos) {
                std::wstring key   = Trim(line.substr(0, pos));
                std::wstring value = Trim(line.substr(pos + 1));
                settings[key]      = value;
            }
        }

        //--------------------------------------------------------------
        // 出力パスの決定
        // (inputPathがエンジン組み込みアセット由来の絶対パスの場合、そのまま
        //  outputDirectory / inputPath とすると絶対パスへ丸ごと置き換わってしまい、
        //  エンジンのソースツリー内に.spritefontを書き込んでしまう。
        //  ToEngineRelativePath()で相対パスに戻してから結合する)
        //--------------------------------------------------------------
        Tsukino::Core::Path outputPath = outputDirectory / Tsukino::IO::FileSystem::ToEngineRelativePath(inputPath);
        outputPath.replace_extension(".spritefont");
        // 出力先を作れないまま書き込みへ進むと、失敗が原因から遠い場所で
        // 「キャッシュが無い」として現れるため、ここで止める
        if(!Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path())) {
            Tsukino::Core::Log::Error("FontImporter: Failed to create the output directory: "
                                      + outputPath.parent_path().string());
            return false;
        }

        //--------------------------------------------------------------
        // MakeSpriteFont.exe のパス
        // (エンジン自身が所有するツールのため、取り込み側リポジトリの
        //  GetAssetRootPath()ではなくGetEngineAssetRootPath()から解決する)
        //--------------------------------------------------------------
        Tsukino::Core::Path toolPath = Tsukino::IO::FileSystem::GetEngineAssetRootPath() / "Tools/MakeSpriteFont.exe";

        //--------------------------------------------------------------
        // オプションを連結
        //--------------------------------------------------------------
        std::wstring arguments = L"\"" + settings[L"FaceName"] + L"\" \"" + outputPath.ToWString() + L"\"";

        // オプションを連結
        arguments += L" /FontSize:" + settings[L"Size"];

        if(settings[L"Italic"] == L"true") {
            arguments += L" /FontStyle:Italic";
        }

        std::wstringstream ss(settings[L"CharacterRegion"]);
        std::wstring       segment;
        while(std::getline(ss, segment, L',')) {
            std::wstring trimmed = Trim(segment);
            if(!trimmed.empty()) {
                // カンマ区切りの数だけ /CharacterRegion:32-126 ... と追加される
                arguments += L" /CharacterRegion:" + trimmed;
            }
        }

        //--------------------------------------------------------------
        // ttf/otf から .spritefont 変換
        // 引数形式: MakeSpriteFont.exe "<input>" "<output>"
        //--------------------------------------------------------------
        if(!RunProcess(toolPath, arguments, baseDir)) {
            Tsukino::Core::Log::Error("Failed to convert font: " + absoluteInputPath.string() + " -> " + outputPath.string());
            return false;
        }

        Tsukino::Core::Log::Info("Font imported: " + outputPath.string());
        return true;
    }

}    // namespace Tsukino::Asset

//--------------------------------------------------------------
//! @file   DynamicFontImporter.cpp
//! @brief  動的フォント(.dfont)のインポータークラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Font/DynamicFontImporter.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>

#include <windows.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <string>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    namespace {
        //--------------------------------------------------------------
        //! @brief  文字列の前後の空白をトリムする関数
        //--------------------------------------------------------------
        std::wstring Trim(const std::wstring& s) {
            auto start = s.find_first_not_of(L" \t\r\n");
            auto end   = s.find_last_not_of(L" \t\r\n");
            return (start == std::wstring::npos) ? L"" : s.substr(start, end - start + 1);
        }

        //--------------------------------------------------------------
        //! @brief  wstring を UTF-8 std::string に変換する関数
        //--------------------------------------------------------------
        std::string ToUtf8(const std::wstring& value) {
            if(value.empty())
                return {};
            int size = ::WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
            std::string result(size, '\0');
            ::WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
            return result;
        }
    }    // namespace

    //--------------------------------------------------------------
    //! @brief  動的フォントのインポート関数
    //--------------------------------------------------------------
    bool DynamicFontImporter::Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) {
        //--------------------------------------------------------------
        // 絶対パスを取得
        //--------------------------------------------------------------
        Tsukino::Core::Path baseDir           = Tsukino::IO::FileSystem::GetAssetRootPath();
        Tsukino::Core::Path absoluteInputPath = baseDir / inPutPath;

        //--------------------------------------------------------------
        // 拡張子チェック
        //--------------------------------------------------------------
        std::string ext = inPutPath.extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if(ext != ".dfont")
            return false;

        //--------------------------------------------------------------
        // .dfont ファイルの解析
        //--------------------------------------------------------------
        std::map<std::wstring, std::wstring> settings;
        settings[L"FaceName"] = L"";
        settings[L"Size"]     = L"32";

        std::wifstream file(absoluteInputPath.ToWString());
        if(!file.is_open()) {
            Tsukino::Core::Log::Error("Failed to open .dfont file: " + absoluteInputPath.string());
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

        if(!settings.contains(L"SourceFile") || settings[L"SourceFile"].empty()) {
            Tsukino::Core::Log::Error("DynamicFontImporter: SourceFile is not specified in " + absoluteInputPath.string());
            return false;
        }

        //--------------------------------------------------------------
        // 参照先の ttf/otf を読み込む
        // (.dfont ファイル自身のあるディレクトリ基準で解決する。
        //  ゲーム側アセット・エンジンのビルトインアセットのどちらでも、
        //  .dfont と参照先フォントファイルが同じディレクトリにあれば正しく動く)
        //--------------------------------------------------------------
        Tsukino::Core::Path sourceFontPath = absoluteInputPath.parent_path() / Tsukino::Core::Path(ToUtf8(settings[L"SourceFile"]));

        std::ifstream fontFile(sourceFontPath.string(), std::ios::binary);
        if(!fontFile.is_open()) {
            Tsukino::Core::Log::Error("DynamicFontImporter: Failed to open SourceFile: " + sourceFontPath.string());
            return false;
        }

        fontFile.seekg(0, std::ios::end);
        const auto fontFileSize = static_cast<size_t>(fontFile.tellg());
        fontFile.seekg(0, std::ios::beg);

        std::vector<uint8_t> fontData(fontFileSize);
        fontFile.read(reinterpret_cast<char*>(fontData.data()), static_cast<std::streamsize>(fontFileSize));

        //--------------------------------------------------------------
        // 出力パスの決定
        // (.dfont はソースとキャッシュで拡張子が変わらないため、そのまま
        //  outputDirectory / inputPath の相対パスへ書き出す)
        //--------------------------------------------------------------
        Tsukino::Core::Path outputPath = outPutDirectory / Tsukino::IO::FileSystem::ToEngineRelativePath(inPutPath);
        Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path());

        //--------------------------------------------------------------
        // キャッシュファイルを書き出す
        // フォーマット: [u32 faceNameLen][faceName utf-8 bytes][float pixelSize][u32 fontDataLen][fontData bytes]
        //--------------------------------------------------------------
        std::string faceNameUtf8 = ToUtf8(settings[L"FaceName"]);
        float       pixelSize    = std::stof(settings[L"Size"]);

        std::ofstream out(outputPath.string(), std::ios::binary);
        if(!out.is_open()) {
            Tsukino::Core::Log::Error("DynamicFontImporter: Failed to write cache file: " + outputPath.string());
            return false;
        }

        const uint32_t faceNameLen = static_cast<uint32_t>(faceNameUtf8.size());
        const uint32_t fontDataLen = static_cast<uint32_t>(fontData.size());

        out.write(reinterpret_cast<const char*>(&faceNameLen), sizeof(faceNameLen));
        out.write(faceNameUtf8.data(), faceNameLen);
        out.write(reinterpret_cast<const char*>(&pixelSize), sizeof(pixelSize));
        out.write(reinterpret_cast<const char*>(&fontDataLen), sizeof(fontDataLen));
        out.write(reinterpret_cast<const char*>(fontData.data()), fontDataLen);

        Tsukino::Core::Log::Info("Dynamic font imported: " + outputPath.string());
        return true;
    }

}    // namespace Tsukino::Asset

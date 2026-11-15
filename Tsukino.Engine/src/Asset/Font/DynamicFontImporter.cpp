//--------------------------------------------------------------
//! @file   DynamicFontImporter.cpp
//! @brief  動的フォント(.dfont)のインポータークラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Font/DynamicFontImporter.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    namespace {
        //--------------------------------------------------------------
        //! @brief  文字列の前後の空白をトリムする関数
        //--------------------------------------------------------------
        std::string Trim(const std::string& s) {
            auto start = s.find_first_not_of(" \t\r\n");
            auto end   = s.find_last_not_of(" \t\r\n");
            return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
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
        // .dfont ファイルの解析（UTF-8 の「キー = 値」の行）
        //--------------------------------------------------------------
        std::map<std::string, std::string> settings;
        settings["FaceName"] = "";
        settings["Size"]     = "32";

        if(!Tsukino::IO::FileSystem::Exists(absoluteInputPath)) {
            Tsukino::Core::Log::Error("Failed to open .dfont file: " + absoluteInputPath.string());
            return false;
        }

        std::istringstream file(Tsukino::IO::FileSystem::ReadText(absoluteInputPath));
        std::string        line;
        while(std::getline(file, line)) {
            if(line.size() >= 3 && line.compare(0, 3, "\xEF\xBB\xBF") == 0)
                line.erase(0, 3);    // 先頭の BOM
            if(line.empty() || line[0] == '#')
                continue;    // 空行とコメントをスキップ

            auto pos = line.find('=');
            if(pos != std::string::npos) {
                std::string key   = Trim(line.substr(0, pos));
                std::string value = Trim(line.substr(pos + 1));
                settings[key]     = value;
            }
        }

        if(!settings.contains("SourceFile") || settings["SourceFile"].empty()) {
            Tsukino::Core::Log::Error("DynamicFontImporter: SourceFile is not specified in " + absoluteInputPath.string());
            return false;
        }

        //--------------------------------------------------------------
        // 参照先の ttf/otf を読み込む
        // (.dfont ファイル自身のあるディレクトリ基準で解決する。
        //  ゲーム側アセット・エンジンのビルトインアセットのどちらでも、
        //  .dfont と参照先フォントファイルが同じディレクトリにあれば正しく動く)
        //--------------------------------------------------------------
        Tsukino::Core::Path sourceFontPath = absoluteInputPath.parent_path() / Tsukino::Core::Path(settings["SourceFile"]);

        const std::vector<uint8_t> fontData = Tsukino::IO::FileSystem::ReadBinary(sourceFontPath);
        if(fontData.empty()) {
            Tsukino::Core::Log::Error("DynamicFontImporter: Failed to open SourceFile: " + sourceFontPath.string());
            return false;
        }

        //--------------------------------------------------------------
        // 出力パスの決定
        // (.dfont はソースとキャッシュで拡張子が変わらないため、そのまま
        //  outputDirectory / inputPath の相対パスへ書き出す)
        //--------------------------------------------------------------
        Tsukino::Core::Path outputPath = outPutDirectory / Tsukino::IO::FileSystem::ToEngineRelativePath(inPutPath);
        // 出力先を作れないまま書き込みへ進むと、失敗が原因から遠い場所で
        // 「キャッシュが無い」として現れるため、ここで止める
        if(!Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path())) {
            Tsukino::Core::Log::Error("DynamicFontImporter: Failed to create the output directory: "
                                      + outputPath.parent_path().string());
            return false;
        }

        //--------------------------------------------------------------
        // キャッシュファイルを書き出す
        // フォーマット: [u32 faceNameLen][faceName utf-8 bytes][float pixelSize][u32 fontDataLen][fontData bytes]
        //--------------------------------------------------------------
        const std::string& faceNameUtf8 = settings["FaceName"];
        const float        pixelSize    = std::stof(settings["Size"]);

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

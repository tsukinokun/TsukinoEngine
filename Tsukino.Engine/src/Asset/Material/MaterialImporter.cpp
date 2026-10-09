//----------------------------------------------------------------------------
//! @file   MaterialImporter.cpp
//! @brief  マテリアルファイル（.tmat）のインポーターの実装
//----------------------------------------------------------------------------
#include <Tsukino/Engine/Asset/Material/MaterialImporter.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>
// MaterialData の hlslpp::interop のシリアライズはここで定義されている
#include <Tsukino/GraphicsCommon/Model/ModelData.hpp>

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    namespace {

        //--------------------------------------------------------------
        //! 文字列の前後の空白を取り除きます。
        //! @param  [in] s 元の文字列
        //! @return 空白を取り除いた文字列
        //--------------------------------------------------------------
        std::string Trim(const std::string& s) {
            const auto start = s.find_first_not_of(" \t\r\n");
            const auto end   = s.find_last_not_of(" \t\r\n");
            return (start == std::string::npos) ? std::string() : s.substr(start, end - start + 1);
        }

        //--------------------------------------------------------------
        //! カンマ区切りの数値を読み取ります。
        //! @param  [in]  text  「1.0, 0.5, 0.2」のような文字列
        //! @param  [in]  count 読み取る個数
        //! @param  [out] out   読み取った値（count 個）
        //! @return count 個すべて読めたら true
        //--------------------------------------------------------------
        bool ParseFloats(const std::string& text, size_t count, float* out) {
            std::stringstream stream(text);
            std::string       item;
            size_t            index = 0;
            while(index < count && std::getline(stream, item, ',')) {
                try {
                    out[index] = std::stof(Trim(item));
                } catch(const std::exception&) {
                    return false;
                }
                ++index;
            }
            return index == count;
        }
    }    // namespace

    //----------------------------------------------------------------------------
    //! マテリアルファイルをインポートします。
    //----------------------------------------------------------------------------
    bool MaterialImporter::Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) {
        const Tsukino::Core::Path absoluteInputPath = Tsukino::IO::FileSystem::GetAssetRootPath() / inPutPath;

        std::ifstream file(absoluteInputPath.string());
        if(!file.is_open()) {
            Tsukino::Core::Log::Error("MaterialImporter: Failed to open " + absoluteInputPath.string());
            return false;
        }

        //--------------------------------------------------------------
        // 「キー = 値」を1行ずつ読む。読めない値・知らないキーは警告して既定値のままにする
        //--------------------------------------------------------------
        Tsukino::GraphicsCommon::MaterialData data;
        data.name = inPutPath.filename();

        auto warn = [&](int lineNumber, const std::string& text) {
            Tsukino::Core::Log::Warn("MaterialImporter: " + text + " (" + inPutPath.string() + " line " + std::to_string(lineNumber) + "). The default value is used.");
        };

        std::string line;
        int         lineNumber = 0;
        while(std::getline(file, line)) {
            ++lineNumber;
            if(const auto comment = line.find('#'); comment != std::string::npos)
                line.erase(comment);
            line = Trim(line);
            if(line.empty())
                continue;

            const auto separator = line.find('=');
            if(separator == std::string::npos) {
                warn(lineNumber, "a line without '='");
                continue;
            }
            const std::string key   = Trim(line.substr(0, separator));
            const std::string value = Trim(line.substr(separator + 1));

            float v[4] = {};
            if(key == "ShadingModel") {
                if(value == "PBR")
                    data.shadingModel = Tsukino::GraphicsCommon::ShadingModel::PBR;
                else if(value == "Unlit")
                    data.shadingModel = Tsukino::GraphicsCommon::ShadingModel::Unlit;
                else if(value == "Toon")
                    data.shadingModel = Tsukino::GraphicsCommon::ShadingModel::Toon;
                else
                    warn(lineNumber, "unknown ShadingModel \"" + value + "\"");
            } else if(key == "BaseColor") {
                if(ParseFloats(value, 4, v))
                    data.baseColor = hlslpp::float4(v[0], v[1], v[2], v[3]);
                else
                    warn(lineNumber, "BaseColor needs 4 numbers");
            } else if(key == "Emissive") {
                if(ParseFloats(value, 3, v))
                    data.emissive = hlslpp::float3(v[0], v[1], v[2]);
                else
                    warn(lineNumber, "Emissive needs 3 numbers");
            } else if(key == "ToonShadeColor") {
                if(ParseFloats(value, 3, v))
                    data.toonShadeColor = hlslpp::float3(std::clamp(v[0], 0.0f, 1.0f), std::clamp(v[1], 0.0f, 1.0f), std::clamp(v[2], 0.0f, 1.0f));
                else
                    warn(lineNumber, "ToonShadeColor needs 3 numbers");
            } else if(key == "ToonThreshold" || key == "ToonSmoothness" || key == "ToonSpecularSize") {
                if(!ParseFloats(value, 1, v)) {
                    warn(lineNumber, key + " needs a number");
                    continue;
                }
                const float clamped = std::clamp(v[0], 0.0f, 1.0f);
                if(key == "ToonThreshold")
                    data.toonThreshold = clamped;
                else if(key == "ToonSmoothness")
                    data.toonSmoothness = clamped;
                else
                    data.toonSpecularSize = clamped;
            } else if(key == "Metallic" || key == "Roughness" || key == "Specular" || key == "AlphaCutoff") {
                if(!ParseFloats(value, 1, v)) {
                    warn(lineNumber, key + " needs a number");
                    continue;
                }
                const float clamped = std::clamp(v[0], 0.0f, 1.0f);
                if(key == "Metallic")
                    data.metallic = clamped;
                else if(key == "Roughness")
                    data.roughness = clamped;
                else if(key == "Specular")
                    data.specular = clamped;
                else
                    data.alphaCutoff = clamped;
            } else if(key == "AlbedoMap") {
                data.albedoMap = value;
            } else if(key == "NormalMap") {
                data.normalMap = value;
            } else if(key == "MetallicRoughnessMap") {
                data.metallicRoughnessMap = value;
            } else if(key == "EmissiveMap") {
                data.emissiveMap = value;
            } else if(key == "AoMap") {
                data.aoMap = value;
            } else {
                warn(lineNumber, "unknown key \"" + key + "\"");
            }
        }

        //--------------------------------------------------------------
        // キャッシュへ書き出す（ソースと同じ相対パス。中身は MaterialData の cereal バイナリ）
        //--------------------------------------------------------------
        const Tsukino::Core::Path outputPath = outPutDirectory / Tsukino::IO::FileSystem::ToEngineRelativePath(inPutPath);
        if(!Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path())) {
            Tsukino::Core::Log::Error("MaterialImporter: Failed to create the output directory: " + outputPath.parent_path().string());
            return false;
        }

        std::ofstream out(outputPath.string(), std::ios::binary);
        if(!out.is_open()) {
            Tsukino::Core::Log::Error("MaterialImporter: Failed to write cache file: " + outputPath.string());
            return false;
        }
        cereal::BinaryOutputArchive archive(out);
        archive(data);

        Tsukino::Core::Log::Info("Material imported: " + outputPath.string());
        return true;
    }
}    // namespace Tsukino::Asset

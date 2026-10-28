//----------------------------------------------------------------------------
//! @file   JsonFile.hpp
//! @brief  項目を省略できる JSON の読み書き
//! @detail cereal の JSON で、書かれていない項目を既定値のまま残して読む関数と、
//!         ファイルが無い・壊れているときに例外を投げずに既定値のまま返す読み書きの関数をまとめます。
//!         調整値や設定のように、後から項目が増えるファイルを読むときに使います。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/IO/CerealOptional.hpp>
#include <Tsukino/Core/Log.hpp>

#include <cereal/archives/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

// 名前空間 : Tsukino::IO
namespace Tsukino::IO {

    //! JSON ファイルから値を読みます。読めなければ out を変えずに警告をログへ出します。
    //! @param  [in]     path ファイルのパス
    //! @param  [in]     key  ルートの項目の名前
    //! @param  [in,out] out  読み込み先。呼ぶ前の値が、書かれていない項目の既定値になる
    //! @return 読めたか。
    //! @note   読み込みは out の写しに対して行い、最後まで読めたときだけ out へ戻す（途中で壊れていても out は元のまま）
    template <class T>
    bool TryLoadJson(const std::string& path, const char* key, T& out) {
        std::ifstream stream(path);
        if(!stream.is_open()) {
            Tsukino::Core::Log::Warn("JSON file not found: " + path + " (using default values)");
            return false;
        }
        T value = out;
        try {
            cereal::JSONInputArchive archive(stream);
            archive(cereal::make_nvp(key, value));
        } catch(const std::exception& e) {
            Tsukino::Core::Log::Warn("Failed to read JSON file: " + path + " (" + e.what() + "; using default values)");
            return false;
        }
        out = std::move(value);
        return true;
    }

    //! 値を JSON ファイルへ書きます。足りないフォルダは作ります。
    //! @param  [in] path  ファイルのパス
    //! @param  [in] key   ルートの項目の名前
    //! @param  [in] value 書き込む値
    //! @return 書けたか。
    template <class T>
    bool TrySaveJson(const std::string& path, const char* key, const T& value) {
        std::error_code error;
        const std::filesystem::path directory = std::filesystem::path(path).parent_path();
        if(!directory.empty()) {
            std::filesystem::create_directories(directory, error);
        }
        std::ofstream stream(path);
        if(!stream.is_open()) {
            Tsukino::Core::Log::Warn("Failed to open JSON file for writing: " + path);
            return false;
        }
        try {
            cereal::JSONOutputArchive archive(stream);
            archive(cereal::make_nvp(key, value));
        } catch(const std::exception& e) {
            Tsukino::Core::Log::Warn("Failed to write JSON file: " + path + " (" + e.what() + ")");
            return false;
        }
        return true;
    }

}    // namespace Tsukino::IO

//-------------------------------------------------------------
//! @file   FileSystem.cpp
//! @brief  FileSystem の実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Core/IO/FileSystem.hpp>
#include <fstream>
#include <filesystem>
// 名前空間 : Tsukino::IO
namespace Tsukino::IO {
    //-------------------------------------------------------------
    //! @brief  バイナリファイルを読み込む
    //-------------------------------------------------------------
    std::vector<std::uint8_t> FileSystem::ReadBinary(const std::string& path) noexcept {
        // 読み込んだバイト列を格納するバッファ
        std::vector<std::uint8_t> buffer;

        // ファイルをバイナリモードで開き、サイズを取得
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if(!file.is_open()) {
            return buffer;
        }

        // ファイルサイズを取得
        const std::streamsize size = file.tellg();
        if(size <= 0) {
            return buffer;
        }

        // バッファをファイルサイズに合わせてリサイズし、ファイルの内容を読み込む
        buffer.resize(static_cast<std::size_t>(size));
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(buffer.data()), size);

        // 読み込みに失敗した場合は空のバッファを返す
        return buffer;
    }

    //-------------------------------------------------------------
    //! @brief  テキストファイルを読み込む
    //-------------------------------------------------------------
    std::string FileSystem::ReadText(const std::string& path) noexcept {
        // ファイルをテキストモードで開き、内容を文字列として読み込む
        std::ifstream file(path);
        // ファイルが開けない場合は空の文字列を返す
        if(!file.is_open()) {
            return std::string();
        }
        // ファイル全体を文字列として読み込む
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    //-------------------------------------------------------------
    //! @brief  ファイルが存在するか確認する
    //-------------------------------------------------------------
    bool FileSystem::Exists(const std::string& path) noexcept {
        // std::filesystemを使用してファイルの存在を確認する
        return std::filesystem::exists(path);
    }

}    // namespace Tsukino::IO

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
    std::vector<std::uint8_t> FileSystem::ReadBinary(const Tsukino::Core::Path& path) noexcept {
        // path.string() を使用
        std::vector<std::uint8_t> buffer;

        // ios::binary で開き、末尾(ate)に移動
        std::ifstream file(path.string(), std::ios::binary | std::ios::ate);
        if(!file)
            return buffer;    // is_open() より file自体の評価が一般的

        // ファイルサイズを取得
        const auto size = file.tellg();
        if(size <= 0)
            return buffer;

        // バッファをサイズに合わせてリサイズし、先頭に移動
        buffer.resize(static_cast<std::size_t>(size));
        file.seekg(0, std::ios::beg);

        // 読み込み成功を確認
        if(!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            buffer.clear();
        }

        // ファイルは自動的に閉じられる
        return buffer;
    }

    //-------------------------------------------------------------
    //! @brief  テキストファイルを読み込む
    //-------------------------------------------------------------
    std::string FileSystem::ReadText(const Tsukino::Core::Path& path) noexcept {
        // path.string() を使用
        std::ifstream file(path.string());
        if(!file.is_open())
            return std::string();

        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    //-------------------------------------------------------------
    //! @brief  ファイルが存在するか確認する
    //-------------------------------------------------------------
    bool FileSystem::Exists(const Tsukino::Core::Path& path) noexcept {
        // path.string() を使用
        return std::filesystem::exists(path.string());
    }

}    // namespace Tsukino::IO

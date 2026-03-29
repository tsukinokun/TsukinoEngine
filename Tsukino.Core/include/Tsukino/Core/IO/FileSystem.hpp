//-------------------------------------------------------------
//! @file   FileSystem.hpp
//! @brief  ファイル読み込みの抽象化レイヤー
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <Tsukino/Core/Path.hpp>
// 名前空間 : Tsukino::IO
namespace Tsukino::IO {
    //-------------------------------------------------------------
    //! @class  FileSystem
    //! @brief  ファイルシステムクラス
    //-------------------------------------------------------------
    class FileSystem final {
    public:
        //---------------------------------------------------------
        // バイナリファイルを読み込む
        //! @param  path [in] 読み込むファイルパス
        //! @return 読み込んだバイト列（失敗時は空）
        //---------------------------------------------------------
        [[nodiscard]]
        static std::vector<std::uint8_t> ReadBinary(const Tsukino::Core::Path& path) noexcept;

        //---------------------------------------------------------
        // テキストファイルを読み込む
        //! @param  path [in] 読み込むファイルパス
        //! @return 読み込んだ文字列（失敗時は空）
        //---------------------------------------------------------
        [[nodiscard]]
        static std::string ReadText(const Tsukino::Core::Path& path) noexcept;

        //---------------------------------------------------------
        // ファイルが存在するか確認する
        //! @param  path [in] 確認するファイルパス
        //---------------------------------------------------------
        [[nodiscard]]
        static bool Exists(const Tsukino::Core::Path& path) noexcept;

        //---------------------------------------------------------
        // ディレクトリを作成する
        //! @param  path [in] 作成するディレクトリのパス
        //! @return ディレクトリの作成に成功した場合は true、すでに存在する場合も true、失敗した場合は false
        //---------------------------------------------------------
        [[nodiscard]]
        static bool CreateDirectories(const Tsukino::Core::Path& path) noexcept;

        //---------------------------------------------------------
        // ファイルの最終更新日時を取得する
        //! @param  path [in] 対象のファイルパス
        //! @return ファイルの最終更新日時（失敗時はデフォルト)
        //---------------------------------------------------------
        std::filesystem::file_time_type GetLastWriteTime(const Tsukino::Core::Path& path) noexcept;
    };

}    // namespace Tsukino::IO

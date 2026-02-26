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
        //! @brief  バイナリファイルを読み込む
        //! @param  path [in] 読み込むファイルパス
        //! @return 読み込んだバイト列（失敗時は空）
        //---------------------------------------------------------
        [[nodiscard]]
        static std::vector<std::uint8_t> ReadBinary(const Tsukino::Core::Path& path) noexcept;

        //---------------------------------------------------------
        //! @brief  テキストファイルを読み込む
        //! @param  path [in] 読み込むファイルパス
        //! @return 読み込んだ文字列（失敗時は空）
        //---------------------------------------------------------
        [[nodiscard]]
        static std::string ReadText(const Tsukino::Core::Path& path) noexcept;

        //---------------------------------------------------------
        //! @brief  ファイルが存在するか確認する
        //! @param  path [in] 確認するファイルパス
        //---------------------------------------------------------
        [[nodiscard]]
        static bool Exists(const Tsukino::Core::Path& path) noexcept;
    };

}    // namespace Tsukino::IO

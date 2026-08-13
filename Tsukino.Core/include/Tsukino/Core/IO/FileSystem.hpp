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
        [[nodiscard]]
        static std::filesystem::file_time_type GetLastWriteTime(const Tsukino::Core::Path& path) noexcept;

        //---------------------------------------------------------
        // アセットのルートパスを取得する
        //! @return アセットのルートパス
        //---------------------------------------------------------
        [[nodiscard]]
        static Tsukino::Core::Path GetAssetRootPath();

        //---------------------------------------------------------
        // エンジン自身のリソース(Tools/, Tsukino.BuiltIn/Assets/等)のルートパスを取得する
        //! @return Debug: エンジンのソースツリー上の絶対パス（コンパイル時にPremakeが注入）
        //!         Release: GetAssetRootPath()と同じ（exeの隣に配置される配布物を前提とする）
        //---------------------------------------------------------
        [[nodiscard]]
        static Tsukino::Core::Path GetEngineAssetRootPath();

        //---------------------------------------------------------
        // 絶対パスをエンジンルート(GetEngineAssetRootPath())からの相対パスへ変換する
        //! @param  maybeAbsolutePath [in] 絶対または相対のパス
        //! @return 絶対パスでエンジンルート配下にある場合は相対パスに変換したもの。
        //!         それ以外（相対パスのまま、またはエンジンルート配下でない絶対パス）は
        //!         そのまま返す。
        //---------------------------------------------------------
        [[nodiscard]]
        static Tsukino::Core::Path ToEngineRelativePath(const Tsukino::Core::Path& maybeAbsolutePath);
    };

}    // namespace Tsukino::IO

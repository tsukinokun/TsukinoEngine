//-------------------------------------------------------------
//! @file   FileSystem.hpp
//! @brief  ファイル読み込みの抽象化レイヤー
//! @author 山﨑愛
//! @note   パスはすべて UTF-8。読み込みは SDL の入出力を通すので、Android ではアプリに同梱した
//!         アセット（APK の中）も、ほかの OS と同じ相対パスで読める
//-------------------------------------------------------------
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
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
        // バイナリファイルを書き込む（足りないフォルダは作る。あれば上書き）
        //! @param  path [in] 書き込むファイルパス
        //! @param  data [in] 書き込むデータ
        //! @param  size [in] data のバイト数
        //! @return 書けたら true
        //---------------------------------------------------------
        [[nodiscard]]
        static bool WriteBinary(const Tsukino::Core::Path& path, const void* data, std::size_t size) noexcept;

        //---------------------------------------------------------
        // テキストファイルを書き込む（足りないフォルダは作る。あれば上書き。改行は変換しない）
        //! @param  path [in] 書き込むファイルパス
        //! @param  text [in] 書き込む文字列
        //! @return 書けたら true
        //---------------------------------------------------------
        [[nodiscard]]
        static bool WriteText(const Tsukino::Core::Path& path, std::string_view text) noexcept;

        //---------------------------------------------------------
        // ファイルが存在するか確認する
        //! @param  path [in] 確認するファイルパス
        //! @return あれば true（Android では APK に同梱したアセットも含む）
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

        //---------------------------------------------------------
        // ユーザーごとのデータ（セーブ・設定）を置くフォルダを取得する
        //! @param  organization [in] 作り手の名前（フォルダ名になる）
        //! @param  application  [in] アプリの名前（フォルダ名になる）
        //! @return フォルダのパス（無ければ作られる）。取れなければ GetAssetRootPath() / "Saves"
        //! @note   Windows は %APPDATA%、macOS は ~/Library/Application Support、
        //!         iOS / Android はアプリ専用の書き込める場所。exe の隣は書き込めないことがあるので使わない
        //---------------------------------------------------------
        [[nodiscard]]
        static Tsukino::Core::Path GetUserDataPath(const std::string& organization, const std::string& application);
    };

}    // namespace Tsukino::IO

//-------------------------------------------------------------
//! @file   FileSystem.cpp
//! @brief  FileSystem の実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Core/IO/FileSystem.hpp>

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>

#include <filesystem>

#if defined(__APPLE__)
#    include <TargetConditionals.h>
#endif

//-------------------------------------------------------------
// アセットをソースの場所から読むか（デスクトップの Debug だけ）。
// スマホでは開発機のパスが見えないので、Debug でも同梱したアセット（.app の中・APK）を読む
//-------------------------------------------------------------
#if defined(_DEBUG) && !defined(__ANDROID__) && !(defined(__APPLE__) && TARGET_OS_IPHONE)
#    define TSUKINO_ASSETS_FROM_SOURCE_TREE 1
#else
#    define TSUKINO_ASSETS_FROM_SOURCE_TREE 0
#endif

// 名前空間 : Tsukino::IO
namespace Tsukino::IO {
    namespace {
        //-------------------------------------------------------------
        //! SDL でファイルを丸ごと読みます。
        //! @param  [in]  path 読むファイルのパス（UTF-8）
        //! @param  [out] size 読んだバイト数
        //! @return 読んだデータ（SDL_free で解放する）。読めなければ nullptr
        //-------------------------------------------------------------
        void* LoadWholeFile(const Tsukino::Core::Path& path, std::size_t& size) noexcept {
            size = 0;
            return SDL_LoadFile(path.string().c_str(), &size);
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief  バイナリファイルを読み込む
    //-------------------------------------------------------------
    std::vector<std::uint8_t> FileSystem::ReadBinary(const Tsukino::Core::Path& path) noexcept {
        std::vector<std::uint8_t> buffer;
        std::size_t               size = 0;
        void*                     data = LoadWholeFile(path, size);
        if(!data)
            return buffer;

        try {
            const auto* bytes = static_cast<const std::uint8_t*>(data);
            buffer.assign(bytes, bytes + size);
        } catch(...) {
            buffer.clear();    // メモリが足りない
        }
        SDL_free(data);
        return buffer;
    }

    //-------------------------------------------------------------
    //! @brief  バイナリファイルの先頭だけを読み込む
    //-------------------------------------------------------------
    std::vector<std::uint8_t> FileSystem::ReadBinaryHead(const Tsukino::Core::Path& path, std::size_t maxBytes) noexcept {
        std::vector<std::uint8_t> buffer;
        SDL_IOStream*             stream = SDL_IOFromFile(path.string().c_str(), "rb");
        if(!stream)
            return buffer;

        try {
            buffer.resize(maxBytes);
            buffer.resize(SDL_ReadIO(stream, buffer.data(), maxBytes));
        } catch(...) {
            buffer.clear();
        }
        SDL_CloseIO(stream);
        return buffer;
    }

    //-------------------------------------------------------------
    //! @brief  テキストファイルを読み込む
    //-------------------------------------------------------------
    std::string FileSystem::ReadText(const Tsukino::Core::Path& path) noexcept {
        std::string text;
        std::size_t size = 0;
        void*       data = LoadWholeFile(path, size);
        if(!data)
            return text;

        try {
            text.assign(static_cast<const char*>(data), size);
            // 改行は "\n" にそろえる（Windows で書いた "\r\n" のファイルも同じに読む）
            std::erase(text, '\r');
        } catch(...) {
            text.clear();
        }
        SDL_free(data);
        return text;
    }

    //-------------------------------------------------------------
    //! @brief  バイナリファイルを書き込む
    //-------------------------------------------------------------
    bool FileSystem::WriteBinary(const Tsukino::Core::Path& path, const void* data, std::size_t size) noexcept {
        const Tsukino::Core::Path directory = path.parent_path();
        if(!directory.string().empty() && !CreateDirectories(directory))
            return false;

        SDL_IOStream* stream = SDL_IOFromFile(path.string().c_str(), "wb");
        if(!stream)
            return false;

        const bool written = (size == 0) || SDL_WriteIO(stream, data, size) == size;
        const bool closed  = SDL_CloseIO(stream);
        return written && closed;
    }

    //-------------------------------------------------------------
    //! @brief  テキストファイルを書き込む
    //-------------------------------------------------------------
    bool FileSystem::WriteText(const Tsukino::Core::Path& path, std::string_view text) noexcept {
        return WriteBinary(path, text.data(), text.size());
    }

    //-------------------------------------------------------------
    //! @brief  ファイルが存在するか確認する
    //-------------------------------------------------------------
    bool FileSystem::Exists(const Tsukino::Core::Path& path) noexcept {
        std::error_code ec;
        if(std::filesystem::exists(path.ToStdPath(), ec))
            return true;

        // OS のファイルとしては見えないもの（Android の APK に同梱したアセット）は、SDL で開けるかで確かめる
        if(SDL_IOStream* stream = SDL_IOFromFile(path.string().c_str(), "rb")) {
            SDL_CloseIO(stream);
            return true;
        }
        return false;
    }

    //---------------------------------------------------------
    //! @brief  ディレクトリを作成する
    //---------------------------------------------------------
    bool FileSystem::CreateDirectories(const Tsukino::Core::Path& path) noexcept {
        //-----------------------------------------------------------
        // std::filesystem::create_directories は「新しく作った」ときだけ
        // true を返し、既にディレクトリがある場合は false を返す。
        // それをそのまま返すと、ヘッダに書いてある「すでに存在する場合も true」
        // という契約に反する。
        //
        // 呼び出し側が戻り値を捨てていた間は無害だったが、検査するように
        // した途端、2回目以降の起動でインポートが全て失敗するようになった。
        // 「作らなかった」と「作れなかった」を error_code で区別すること
        //-----------------------------------------------------------
        std::error_code ec;

        if(std::filesystem::create_directories(path.ToStdPath(), ec)) {
            return true;    // 新規に作成できた
        }

        if(ec) {
            return false;    // 実際に失敗した
        }

        // 何も作らなかった場合。既にディレクトリとして在るなら成功とみなす
        return std::filesystem::is_directory(path.ToStdPath(), ec);
    }

    //---------------------------------------------------------
    //! @brief  ファイルの最終更新日時を取得する
    //---------------------------------------------------------
    std::filesystem::file_time_type FileSystem::GetLastWriteTime(const Tsukino::Core::Path& path) noexcept {
        std::error_code ec;
        const auto      time = std::filesystem::last_write_time(path.ToStdPath(), ec);
        // 失敗時（無いファイル・APK の中など）は最小値を返し、比較で「古い」と判定させる
        return ec ? std::filesystem::file_time_type::min() : time;
    }

    //---------------------------------------------------------
    //! @brief  アセットのルートパスを取得する
    //---------------------------------------------------------
    Tsukino::Core::Path FileSystem::GetAssetRootPath() {
#if TSUKINO_ASSETS_FROM_SOURCE_TREE
        // デバッグ時はプロジェクトルート（Premakeのdebugdir・CMakeの作業フォルダ設定に依存）
        std::error_code ec;
        return Tsukino::Core::Path(std::filesystem::current_path(ec));
#elif defined(__ANDROID__)
        // Android はアセットを APK の直下に入れる。SDL は相対パスを APK の中から読むので、ルートは空にする
        return Tsukino::Core::Path("");
#else
        // リリース時は実行ファイル (.exe) の場所を基準にする。
        // SDL_GetBasePath は UTF-8 で、末尾に区切り文字が付く（macOS の .app では Resources フォルダ、iOS は .app を指す）
        const char* basePath = SDL_GetBasePath();
        if(!basePath) {
            std::error_code ec;
            return Tsukino::Core::Path(std::filesystem::current_path(ec));
        }
        std::string root(basePath);
        while(!root.empty() && (root.back() == '/' || root.back() == '\\')) {
            root.pop_back();
        }
        return Tsukino::Core::Path(root);
#endif
    }

    //---------------------------------------------------------
    //! @brief  エンジン自身のリソースのルートパスを取得する
    //---------------------------------------------------------
    Tsukino::Core::Path FileSystem::GetEngineAssetRootPath() {
#if TSUKINO_ASSETS_FROM_SOURCE_TREE
        // デバッグ時：エンジン自身のソースツリー上の絶対パス（Premakeがコンパイル時に注入）を直接参照する。
        // 取り込み側リポジトリのルートへTools/やTsukino.BuiltIn/Assetsをコピー・リンクする必要がなくなる。
        return Tsukino::Core::Path(TSUKINO_ENGINE_ROOT);
#else
        // リリース時：GetAssetRootPath()と同じ（exeの隣に配置される配布物を前提とする）
        return GetAssetRootPath();
#endif
    }

    //---------------------------------------------------------
    //! @brief  絶対パスをエンジンルートからの相対パスへ変換する
    //---------------------------------------------------------
    Tsukino::Core::Path FileSystem::ToEngineRelativePath(const Tsukino::Core::Path& maybeAbsolutePath) {
        const std::filesystem::path srcFsPath = maybeAbsolutePath.ToStdPath();
        if(!srcFsPath.is_absolute()) {
            // 相対パスはそのまま返す（従来通りの挙動。ゲーム自身のアセット等はここに該当）
            return maybeAbsolutePath;
        }

        const std::filesystem::path engineRoot = GetEngineAssetRootPath().ToStdPath();
        std::error_code             ec;
        const std::filesystem::path rel = std::filesystem::relative(srcFsPath, engineRoot, ec);
        if(ec || rel.empty() || rel.native()[0] == '.') {
            // エンジンルート配下でない絶対パスは変換できないためそのまま返す
            return maybeAbsolutePath;
        }

        return Tsukino::Core::Path(rel);
    }

    //---------------------------------------------------------
    //! @brief  ユーザーごとのデータを置くフォルダを取得する
    //---------------------------------------------------------
    Tsukino::Core::Path FileSystem::GetUserDataPath(const std::string& organization, const std::string& application) {
        char* prefPath = SDL_GetPrefPath(organization.c_str(), application.c_str());
        if(!prefPath)
            return GetAssetRootPath() / "Saves";

        std::string path(prefPath);
        SDL_free(prefPath);
        while(!path.empty() && (path.back() == '/' || path.back() == '\\')) {
            path.pop_back();
        }
        return Tsukino::Core::Path(path);
    }
}    // namespace Tsukino::IO

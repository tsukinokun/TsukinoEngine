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

    //---------------------------------------------------------
    //! @brief  ディレクトリを作成する
    //---------------------------------------------------------
    bool FileSystem::CreateDirectories(const Tsukino::Core::Path& path) noexcept {
        // path.string() を使用
        try {
            return std::filesystem::create_directories(path.string());
        } catch(...) {
            return false;
        }
    }

    //---------------------------------------------------------
    //! @brief  ファイルの最終更新日時を取得する
    //---------------------------------------------------------
    std::filesystem::file_time_type FileSystem::GetLastWriteTime(const Tsukino::Core::Path& path) noexcept {
        try {
            if(Exists(path)) {
                return std::filesystem::last_write_time(path.string());
            }
        } catch(...) {
            // エラー時はエポック（古い時間）を返すなどして、比較で「古い」と判定させる
        }
        // 失敗時は最小値を返す
        return std::filesystem::file_time_type::min();
    }

    //---------------------------------------------------------
    //! @brief  アセットのルートパスを取得する
    //---------------------------------------------------------
    Tsukino::Core::Path FileSystem::GetAssetRootPath() {
#ifdef _DEBUG
        // デバッグ時はプロジェクトルート（Premakeのdebugdir設定に依存）
        return Tsukino::Core::Path(std::filesystem::current_path().string());
#else
        // リリース時は実行ファイル (.exe) の場所を基準にする
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        return Tsukino::Core::Path(std::filesystem::path(exePath).parent_path().string());
#endif
    }

    //---------------------------------------------------------
    //! @brief  エンジン自身のリソースのルートパスを取得する
    //---------------------------------------------------------
    Tsukino::Core::Path FileSystem::GetEngineAssetRootPath() {
#ifdef _DEBUG
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
        std::filesystem::path srcFsPath(maybeAbsolutePath.string());
        if(!srcFsPath.is_absolute()) {
            // 相対パスはそのまま返す（従来通りの挙動。ゲーム自身のアセット等はここに該当）
            return maybeAbsolutePath;
        }

        std::filesystem::path engineRoot(GetEngineAssetRootPath().string());
        std::error_code       ec;
        std::filesystem::path rel = std::filesystem::relative(srcFsPath, engineRoot, ec);
        if(ec || rel.empty() || rel.native()[0] == L'.') {
            // エンジンルート配下でない絶対パスは変換できないためそのまま返す
            return maybeAbsolutePath;
        }

        return Tsukino::Core::Path(rel.generic_string());
    }
}    // namespace Tsukino::IO

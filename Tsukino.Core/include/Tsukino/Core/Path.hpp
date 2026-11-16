//--------------------------------------------------------------
//! @file   Path.hpp
//! @brief  パス管理クラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <utility>
// 名前空間 : Tsukino::Core
namespace Tsukino::Core {
    //--------------------------------------------------------------
    //! @class   Path
    //! @brief   パス管理クラス
    //! @note    パスは UTF-8 の文字列で持ち、区切りは '/' にそろえる。
    //!          OS のファイル操作へ渡すときは ToStdPath() を使う（Windows でも日本語のパスが化けない）
    //--------------------------------------------------------------
    class Path {
    public:
        //--------------------------------------------------------------
        //! @brief   コンストラクタ
        //--------------------------------------------------------------
        Path() = default;

        //--------------------------------------------------------------
        //! @brief   コンストラクタ
        //! @param   path [in] パス文字列（UTF-8）
        //--------------------------------------------------------------
        Path(const std::string& path)
            : m_path(Normalize(path)) {}

        //--------------------------------------------------------------
        //! @brief   コンストラクタ
        //! @param   path [in] パス文字列（UTF-8。終端あり）
        //--------------------------------------------------------------
        Path(const char* path)
            : m_path(Normalize(path ? std::string(path) : std::string())) {}

        //--------------------------------------------------------------
        //! @brief   std::filesystem::path から作るコンストラクタ
        //! @param   path [in] OS のパス
        //--------------------------------------------------------------
        explicit Path(const std::filesystem::path& path)
            : m_path(FromStdPath(path)) {}

        //--------------------------------------------------------------
        //! @brief  パス文字列を取得する関数
        //! @return パス文字列
        //--------------------------------------------------------------
        [[nodiscard]]
        const std::string& string() const noexcept {
            return m_path;
        }

        //--------------------------------------------------------------
        //! @brief  パスの親ディレクトリを取得する関数
        //! @return 親ディレクトリのパス
        //--------------------------------------------------------------
        [[nodiscard]]
        Path parent() const {
            return Path(ToStdPath().parent_path());
        }

        //--------------------------------------------------------------
        //! @brief  パスのファイル名を取得する関数
        //! @return ファイル名
        //--------------------------------------------------------------
        [[nodiscard]]
        std::string filename() const {
            return FromStdPath(ToStdPath().filename());
        }

        //--------------------------------------------------------------
        //! @brief  パスの拡張子を取得する関数
        //! @return 拡張子（例: ".png"）
        //--------------------------------------------------------------
        [[nodiscard]]
        std::string extension() const {
            return FromStdPath(ToStdPath().extension());
        }

        //--------------------------------------------------------------
        //! @brief  拡張子を除いたファイル名を取得する関数
        //! @return 拡張子なしのファイル名
        //--------------------------------------------------------------
        [[nodiscard]]
        std::string stem() const {
            return FromStdPath(ToStdPath().stem());
        }

        //--------------------------------------------------------------
        //! @brief  パスの親ディレクトリを取得する関数
        //! @return 親ディレクトリのパス
        //--------------------------------------------------------------
        [[nodiscard]]
        Path parent_path() const {
            return Path(ToStdPath().parent_path());
        }

        //--------------------------------------------------------------
        //! @brief  OS のファイル操作に渡せるパスを取得する関数
        //! @return std::filesystem::path（UTF-8 として解釈したもの）
        //! @note   std::ifstream などへ string() をそのまま渡すと、Windows では UTF-8 ではなく
        //!         ANSI のコードページとして読まれ、日本語を含むパスが開けない。必ずこちらを渡す
        //--------------------------------------------------------------
        [[nodiscard]]
        std::filesystem::path ToStdPath() const {
            return std::filesystem::path(std::u8string(m_path.begin(), m_path.end()));
        }

        //-----------------------------------------------------------
        //! @brief  パスを結合する演算子
        //! @param  rhs [in] 結合するパス要素（文字列からも暗黙に変わる）
        //! @return 結合後のパス
        //-----------------------------------------------------------
        [[nodiscard]]
        Path operator/(const Path& rhs) const {
            return Path(ToStdPath() / rhs.ToStdPath());
        }

        //-----------------------------------------------------------
        //! @brief  拡張子を置き換える関数
        //! @param  newExt [in] 新しい拡張子
        //-----------------------------------------------------------
        void replace_extension(const std::string& newExt) {
            std::filesystem::path p = ToStdPath();
            p.replace_extension(Path(newExt).ToStdPath());
            m_path = FromStdPath(p);
        }

        //--------------------------------------------------------------
        //! @brief  フラグメントなしとフラグメントを分割して取得する関数
        //! @return フラグメントなしのパスとフラグメントのペア
        //--------------------------------------------------------------
        [[nodiscard]]
        static std::pair<std::string, std::string> SplitPathAndFragment(const std::string& rawPath) {
            const size_t pos = rawPath.find('#');
            if(pos == std::string::npos)
                return {rawPath, ""};
            return {rawPath.substr(0, pos), rawPath.substr(pos + 1)};
        }

        //--------------------------------------------------------------
        //! @brief  文字列を小文字に変換する関数（ASCII のみ）
        //! @param  value [in] 変換する文字列
        //! @return 小文字にした文字列
        //--------------------------------------------------------------
        [[nodiscard]]
        static std::string ToLower(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return value;
        }

    private:
        std::string m_path;    // 正規化されたパス文字列

        //--------------------------------------------------------------
        //! @brief  パスを正規化する関数
        //! @param  path [in] 正規化する前のパス文字列
        //! @return 正規化されたパス文字列
        //--------------------------------------------------------------
        [[nodiscard]]
        static std::string Normalize(const std::string& path) {
            return FromStdPath(std::filesystem::path(std::u8string(path.begin(), path.end())));
        }

        //--------------------------------------------------------------
        //! @brief  std::filesystem::path を UTF-8 の文字列（区切りは '/'）にする関数
        //! @param  path [in] OS のパス
        //! @return UTF-8 のパス文字列
        //--------------------------------------------------------------
        [[nodiscard]]
        static std::string FromStdPath(const std::filesystem::path& path) {
            const std::u8string text = path.generic_u8string();
            return std::string(text.begin(), text.end());
        }

        //--------------------------------------------------------------
        //! @brief フラグメント（#以降）を除いたパスを取得する関数
        //! @return フラグメントなしのパス
        //--------------------------------------------------------------
        [[nodiscard]]
        Path GetBasePath() const {
            size_t pos = m_path.find('#');
            if(pos == std::string::npos)
                return *this;
            return Path(m_path.substr(0, pos));
        }

        //--------------------------------------------------------------
        //! @brief フラグメント（#以降）を取得する関数
        //! @return フラグメント文字列（#は含まない。ない場合は空文字）
        //--------------------------------------------------------------
        [[nodiscard]]
        std::string GetFragment() const {
            size_t pos = m_path.find('#');
            if(pos == std::string::npos)
                return "";
            return m_path.substr(pos + 1);
        }

        //--------------------------------------------------------------
        //! @brief フラグメントを無視して拡張子を取得する関数
        //! @return 正規の拡張子（例: ".png"）
        //--------------------------------------------------------------
        [[nodiscard]]
        std::string GetPureExtension() const {
            return GetBasePath().extension();
        }
    };
}    // namespace Tsukino::Core

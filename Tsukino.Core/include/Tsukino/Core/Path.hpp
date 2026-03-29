//--------------------------------------------------------------
//! @file   Path.hpp
//! @brief  パス管理クラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <string>
#include <filesystem>
// 名前空間 : Tsukino::Core
namespace Tsukino::Core {
    //--------------------------------------------------------------
    //! @class   Path
    //! @brief   パス管理クラス
    //--------------------------------------------------------------
    class Path {
    public:
        //--------------------------------------------------------------
        //! @brief   コンストラクタ
        //--------------------------------------------------------------
        Path() = default;

        //--------------------------------------------------------------
        //! @brief   コンストラクタ
        //! @param   path [in] パス文字列
        //--------------------------------------------------------------
        Path(const std::string& path)
            : m_path(Normalize(path)) {}

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
            return Path(std::filesystem::path(m_path).parent_path().generic_string());
        }

        //--------------------------------------------------------------
        //! @brief  パスのファイル名を取得する関数
        //! @return ファイル名
        //--------------------------------------------------------------
        [[nodiscard]]
        std::string filename() const {
            return std::filesystem::path(m_path).filename().string();
        }

        //--------------------------------------------------------------
        //! @brief  パスの拡張子を取得する関数
        //! @return 拡張子（例: ".png"）
        //--------------------------------------------------------------
        [[nodiscard]]
        std::string extension() const {
            return std::filesystem::path(m_path).extension().string();
        }

        //--------------------------------------------------------------
        //! @brief  拡張子を除いたファイル名を取得する関数
        //! @return 拡張子なしのファイル名
        //--------------------------------------------------------------
        [[nodiscard]]
        std::string stem() const {
            return std::filesystem::path(m_path).stem().string();
        }

        //--------------------------------------------------------------
        //! @brief  パスの親ディレクトリを取得する関数
        //! @return 親ディレクトリのパス
        //--------------------------------------------------------------
        [[nodiscard]]
        Path parent_path() const {
            return Path(std::filesystem::path(m_path).parent_path().generic_string());
        }

        //--------------------------------------------------------------
        //! @brief  パスをwide string (UTF-16) に変換して取得する関数
        //! @return UTF-16 のパス文字列
        //--------------------------------------------------------------
        [[nodiscard]]
        std::wstring ToWString() const {
            return std::filesystem::path(m_path).wstring();
        }

        //-----------------------------------------------------------
        //! @brief  パスを結合する演算子
        //! @param  rhs [in] 結合するパス要素
        //! @return 結合後のパス
        //-----------------------------------------------------------
        [[nodiscard]]
        Path operator/(const std::string& rhs) const {
            return Path((std::filesystem::path(m_path) / rhs).generic_string());
        }

        //-----------------------------------------------------------
        //! @brief  パスを結合する演算子
        //! @param  rhs [in] 結合するパス要素
        //! @return 結合後のパス
        //-----------------------------------------------------------
        [[nodiscard]]
        Path operator/(const Path& rhs) const {
            return Path((std::filesystem::path(m_path) / rhs.m_path).generic_string());
        }

        //-----------------------------------------------------------
        //! @brief  拡張子を置き換える関数
        //! @param  newExt [in] 新しい拡張子
        //-----------------------------------------------------------
        void replace_extension(const std::string& newExt) {
            std::filesystem::path p(m_path);
            p.replace_extension(newExt);
            m_path = p.generic_string();
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
            return std::filesystem::path(path).generic_string();
        }
    };
}    // namespace Tsukino::Core

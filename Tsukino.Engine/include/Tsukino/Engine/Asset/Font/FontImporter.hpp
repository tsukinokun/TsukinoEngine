//--------------------------------------------------------------
//! @file	FontImporter.hpp
//! @brief  フォントのインポータークラス
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetImporter.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class FontImporter
    //! @brief フォントのインポータークラス
    //--------------------------------------------------------------
    class FontImporter : public IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! @brief  フォントアセットをインポートする関数
        //! @param  path            [in] インポートするフォントアセットのパス
        //! @param  outPutDirectory [in] 出力先ディレクトリ
        //! @return インポートされたフォントアセットの参照
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) override;

    private:
        //--------------------------------------------------------------
        // 外部プロセスを実行して終了コードを返す
        //! @param  executablePath [in] 実行ファイルの絶対パス
        //! @param  arguments      [in] 引数文字列
        //! @param  workingDir     [in] カレントディレクトリ
        //! @return 実行成功かつ終了コード0なら true
        //--------------------------------------------------------------
        [[nodiscard]]
        static bool RunProcess(const Tsukino::Core::Path& executablePath, const std::wstring& arguments, const Tsukino::Core::Path& workingDir);
    };

}    // namespace Tsukino::Asset

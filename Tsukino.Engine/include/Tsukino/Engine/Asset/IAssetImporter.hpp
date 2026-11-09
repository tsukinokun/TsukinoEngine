//--------------------------------------------------------------
//! @file	IAssetImporter.hpp
//! @brief  アセットインポーターの共通インターフェース
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Memory.hpp>

#include <vector>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    class IAsset;    // 前方宣言
    //--------------------------------------------------------------
    //! @class IAssetImporter
    //! @brief アセットインポーターの共通インターフェース
    //--------------------------------------------------------------
    class IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! @brief 仮想デストラクタ
        //--------------------------------------------------------------
        virtual ~IAssetImporter() = default;

        //--------------------------------------------------------------
        //! @brief  アセットをインポートする関数
        //! @param  path            [in] インポートするアセットのパス
        //! @param  outPutDirectory [in] インポートされたアセットの出力先ディレクトリ
        //! @return インポートされたアセットの参照
        //--------------------------------------------------------------
        [[nodiscard]]
        virtual bool Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) = 0;

        //--------------------------------------------------------------
        //! ソース以外に、このアセットの中身を左右するファイルを列挙します。
        //! @param  [in] inputPath ソースファイルのパス
        //! @return 依存ファイルの絶対パス一覧。既定では空
        //! @note   AssetManagerはキャッシュの作り直しを「ソースとキャッシュの
        //!         更新日時の比較」で決めるが、それだけではソース本体を直したときしか
        //!         気付けない。シェーダーの.hlsliのように、ソースが取り込んでいる
        //!         別ファイルを直した場合は日時が変わらず、古いキャッシュが
        //!         使われ続けてしまう（コンパイルエラーも出ないので発見が難しい）。
        //!         ここで申告されたファイルも比較対象に加わる
        //--------------------------------------------------------------
        [[nodiscard]]
        virtual std::vector<Tsukino::Core::Path> CollectDependencies(const Tsukino::Core::Path& inputPath) const {
            (void)inputPath;
            return {};
        }

        //--------------------------------------------------------------
        //! 既にあるキャッシュが今の設定に合っていないかを調べます。
        //! @param  [in] inputPath ソースファイルのパス
        //! @param  [in] cachePath キャッシュファイルのパス
        //! @return true: 更新日時に関わらず作り直す, false: 作り直さない（既定）
        //! @note   インポートの設定を変えてもソースの日時は変わらないので、
        //!         設定で中身が変わるアセットはここで気付けるようにする
        //--------------------------------------------------------------
        [[nodiscard]]
        virtual bool IsCacheOutdated(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& cachePath) const {
            (void)inputPath;
            (void)cachePath;
            return false;
        }
    };
}    // namespace Tsukino::Asset

//--------------------------------------------------------------
//! @file	ShaderImporter.hpp
//! @brief  シェーダーのインポータークラス
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetImporter.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class ShaderImporter
    //! @brief シェーダーのインポータークラス
    //--------------------------------------------------------------
    class ShaderImporter : public IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! @brief  シェーダーアセットをインポートする関数
        //! @param  path            [in] インポートするシェーダーアセットのパス
        //! @param  outPutDirectory [in] 出力先ディレクトリ
        //! @return インポートされたシェーダーアセットの参照
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) override;

        //--------------------------------------------------------------
        //! シェーダーが #include している .hlsli を再帰的に列挙します。
        //! @param  [in] inputPath シェーダーソースのパス
        //! @return インクルードしているファイルの絶対パス一覧
        //! @note   これが無いと、共通定義（Lighting.hlsli など）を直しても
        //!         それを取り込む .hlsl の更新日時が変わらないため再コンパイルされず、
        //!         定数バッファのスロット番号を動かしたのに古い .cso が使われて
        //!         「絵だけ壊れる」という事故になる
        //--------------------------------------------------------------
        [[nodiscard]]
        std::vector<Tsukino::Core::Path> CollectDependencies(const Tsukino::Core::Path& inputPath) const override;
    };

}    // namespace Tsukino::Asset

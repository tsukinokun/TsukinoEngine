//--------------------------------------------------------------
//! @file	TextureImporter.hpp
//! @brief  テクスチャのインポータークラス
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetImporter.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureImportRules.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class TextureImporter
    //! @brief テクスチャのインポータークラス
    //--------------------------------------------------------------
    class TextureImporter : public IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! 規則を受け取ります。
        //! @param  [in] rules インポートの規則（nullptr なら規則なし）
        //--------------------------------------------------------------
        explicit TextureImporter(Tsukino::Core::Ref<const TextureImportRules> rules = nullptr)
            : m_rules(std::move(rules)) {
        }

        //--------------------------------------------------------------
        //! 既にあるキャッシュが規則のフォーマットと違うかを調べます。
        //! @param  [in] inputPath ソースファイルのパス
        //! @param  [in] cachePath キャッシュファイルのパス
        //! @return true: 作り直す, false: 作り直さない
        //--------------------------------------------------------------
        [[nodiscard]]
        bool IsCacheOutdated(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& cachePath) const override;

        //--------------------------------------------------------------
        //! @brief  テクスチャアセットをインポートする関数
        //! @param  inputPath       [in] インポートするテクスチャアセットのパス
        //! @param  outputDirectory [in] 出力先ディレクトリ
        //! @return 書き出せたら true
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) override;

    private:
        Tsukino::Core::Ref<const TextureImportRules> m_rules;    // インポートの規則（AssetManager と共有する）
    };

}    // namespace Tsukino::Asset

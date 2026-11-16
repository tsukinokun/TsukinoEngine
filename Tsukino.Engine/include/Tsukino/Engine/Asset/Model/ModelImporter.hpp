//--------------------------------------------------------------
//! @file	ModelImporter.hpp
//! @brief  モデルのインポータークラス
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetImporter.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureImportRules.hpp>
#include <Tsukino/Core/Memory.hpp>

#include <utility>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class ModelImporter
    //! @brief モデルのインポータークラス
    //--------------------------------------------------------------
    class ModelImporter : public IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! @brief  コンストラクタ
        //! @param  rules [in] テクスチャの規則（埋め込み画像の圧縮をどの端末に合わせるかを読む。nullptr ならパソコン向け）
        //--------------------------------------------------------------
        explicit ModelImporter(Tsukino::Core::Ref<const TextureImportRules> rules = nullptr)
            : m_rules(std::move(rules)) {}

        //--------------------------------------------------------------
        //! @brief  モデルアセットをインポートする関数
        //! @param  inputPath       [in] インポートするモデルアセットのパス
        //! @param  outputDirectory [in] 出力先ディレクトリ
        //! @return 書き出せたら true
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) override;

    private:
        Tsukino::Core::Ref<const TextureImportRules> m_rules;    // 圧縮を合わせる端末を知るため（AssetManager と共有する）
    };

}    // namespace Tsukino::Asset

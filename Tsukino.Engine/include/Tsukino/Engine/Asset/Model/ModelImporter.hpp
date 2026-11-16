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
        //! @param  path            [in] インポートするモデルアセットのパス
        //! @param  outPutDirectory [in] 出力先ディレクトリ
        //! @return インポートされたモデルアセットの参照
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) override;

    private:
        Tsukino::Core::Ref<const TextureImportRules> m_rules;    // 圧縮を合わせる端末を知るため（AssetManager と共有する）
    };

}    // namespace Tsukino::Asset

//--------------------------------------------------------------
//! @file   CubemapImporter.hpp
//! @brief  キューブマップのインポータークラス
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
    //! @class CubemapImporter
    //! @brief .cubemapファイルから6枚のPNGを読み込み
    //!        キューブマップ（KTX）として出力するインポータークラス
    //--------------------------------------------------------------
    class CubemapImporter : public IAssetImporter {
    public:
        //--------------------------------------------------------------
        //! @brief  コンストラクタ
        //! @param  rules [in] テクスチャの規則（圧縮をどの端末に合わせるかを読む。nullptr ならパソコン向け）
        //--------------------------------------------------------------
        explicit CubemapImporter(Tsukino::Core::Ref<const TextureImportRules> rules = nullptr)
            : m_rules(std::move(rules)) {}

        //--------------------------------------------------------------
        //! @brief  キューブマップアセットをインポートする関数
        //! @param  inputPath       [in] .cubemapファイルのパス
        //! @param  outputDirectory [in] 出力先ディレクトリ
        //! @return 成功したらtrue
        //--------------------------------------------------------------
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) override;

    private:
        Tsukino::Core::Ref<const TextureImportRules> m_rules;    // 圧縮を合わせる端末を知るため（AssetManager と共有する）
    };
}    // namespace Tsukino::Asset

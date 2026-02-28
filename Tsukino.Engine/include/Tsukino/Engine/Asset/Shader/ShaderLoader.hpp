//--------------------------------------------------------------
//! @file   ShaderLoader.hpp
//! @brief  シェーダアセットローダーの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetLoader.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
#include <Tsukino/Core/Path.hpp>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  ShaderLoader
    //! @brief  .shader ファイルを読み込んでShaderAssetを生成するローダー
    //--------------------------------------------------------------
    class ShaderLoader : public IAssetLoader {
    public:
        //--------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //--------------------------------------------------------------
        ShaderLoader() = default;

        //--------------------------------------------------------------
        // 対応する拡張子か判定する
        //! @param ext [in] 拡張子
        //! @return 対応している場合 true
        //--------------------------------------------------------------
        [[nodiscard]]
        bool CanLoad(const std::string& ext) const override;

        //--------------------------------------------------------------
        // シェーダファイルを読み込みShaderAssetを生成する
        //! @param path [in] シェーダファイルのパス
        //! @return 読み込まれたShaderAsset
        //--------------------------------------------------------------
        [[nodiscard]]
        Tsukino::Core::Ref<IAsset> Load(const Tsukino::Core::Path& path) override;
    };

}    // namespace Tsukino::Asset

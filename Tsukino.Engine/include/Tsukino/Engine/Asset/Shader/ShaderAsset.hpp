//--------------------------------------------------------------
//! @file   ShaderAsset.hpp
//! @brief  シェーダアセットクラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAsset.hpp>
#include <Tsukino/Engine/Asset/AssetType.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderStage.hpp>

#include <string>
#include <vector>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {

    //--------------------------------------------------------------
    //! @class  ShaderAsset
    //! @brief  シェーダアセット（ビルド時に変換した bgfx のシェーダー。CPU 側データ）
    //--------------------------------------------------------------
    class ShaderAsset : public IAsset {
    public:
        //--------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //--------------------------------------------------------------
        ShaderAsset() = default;

        //--------------------------------------------------------------
        //! @brief  アセットの種類を取得
        //! @return アセットの種類
        //--------------------------------------------------------------
        [[nodiscard]]
        AssetType GetType() const override {
            return AssetType::Shader;
        }

        Tsukino::Shader::ShaderStage shaderStage;    // シェーダーステージ
        std::vector<u8>              binary;         // 変換済みのバイナリデータ（bgfx のシェーダーの形式）
        std::string                  filePath;       // 元ファイルのパス
    };

}    // namespace Tsukino::Asset

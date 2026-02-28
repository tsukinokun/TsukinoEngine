//--------------------------------------------------------------
//! @file   ShaderAsset.hpp
//! @brief  シェーダアセットクラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <string>
#include <Tsukino/Engine/Asset/IAsset.hpp>
#include <Tsukino/Engine/Asset/AssetType.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {

    //--------------------------------------------------------------
    //! @class  ShaderAsset
    //! @brief  HLSL シェーダアセット（CPU 側データ）
    //--------------------------------------------------------------
    class ShaderAsset : public IAsset {
    public:
        //--------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //--------------------------------------------------------------
        ShaderAsset() = default;

        //--------------------------------------------------------------
        //! @brief  アセットのハンドルを取得
        //! @return アセットのハンドル
        //--------------------------------------------------------------
        [[nodiscard]]
        AssetHandle GetHandle() const override {
            return m_handle;
        }

        //--------------------------------------------------------------
        //! @brief  アセットの種類を取得
        //! @return アセットの種類
        //--------------------------------------------------------------
        [[nodiscard]]
        AssetType GetType() const override {
            return AssetType::Shader;
        }

        //--------------------------------------------------------------
        //! @brief  ローダー側から設定されるハンドル
        //! @param handle [in] 設定するハンドル
        //--------------------------------------------------------------
        void SetHandle(AssetHandle handle) { m_handle = handle; }

        // エントリーポイント名
        std::string entryVS = "VSMain";
        std::string entryPS = "PSMain";

        std::string source;      // HLSL ソースコード
        std::string filePath;    // 元ファイルのパス

    private:
        AssetHandle m_handle = AssetHandle::Invalid();    // アセットのハンドル
    };

}    // namespace Tsukino::Asset

//--------------------------------------------------------------
//! @file   MaterialAsset.hpp
//! @brief  マテリアルアセットの定義
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/IAsset.hpp>

#include <hlsl++.h>
// 名前空間 ：Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  MaterialAsset
    //! @brief  マテリアルアセットクラス
    //--------------------------------------------------------------
    class MaterialAsset : public IAsset {
    public:
        //--------------------------------------------------------------
        //! @brief  ハンドルを取得する関数
        //! @return アセットのハンドル
        //--------------------------------------------------------------
        [[nodiscard]]
        AssetHandle GetHandle() const override {
            return handle;
        }

        //--------------------------------------------------------------
        //! @brief  アセットの種類を取得する関数
        //! @return アセットの種類
        //--------------------------------------------------------------
        [[nodiscard]]
        AssetType GetType() const override {
            return AssetType::Material;
        }

        //--------------------------------------------------------------
        //! @brief ハンドル設定用のセッター
        //! @param h [in] 設定するハンドル
        //--------------------------------------------------------------
        void SetHandle(AssetHandle h) { handle = h; }

        // テクスチャの配列
        std::vector<AssetHandle> textureHandles;

        AssetHandle    vertexShaderHandle;    // VS
        AssetHandle    pixelShaderHandle;     // PS
        hlslpp::float4 diffuseColor;          // 基本色

    private:
        AssetHandle handle;    // IAssetの実体となる識別子
    };
}    // namespace Tsukino::Asset

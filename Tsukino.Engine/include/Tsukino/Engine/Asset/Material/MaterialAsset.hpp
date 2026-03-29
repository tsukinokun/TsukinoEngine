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
        AssetHandle    textureHandle;         // テクスチャアセットのID
        AssetHandle    vertexShaderHandle;    // バーテックスシェーダーアセットのID
        AssetHandle    pixelShaderHandle;     // ピクセルシェーダーアセットのID
        hlslpp::float4 diffuseColor;          // 色データ
    };
}    // namespace Tsukino::Asset

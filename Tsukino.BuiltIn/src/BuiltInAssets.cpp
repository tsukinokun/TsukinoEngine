//--------------------------------------------------------------
//! @file   BuiltInAssets.cpp
//! @brief  ビルトインアセット集約クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/BuiltIn/BuiltInAssets.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>
// 名前空間 : Tsukino::BuiltIn
namespace Tsukino::BuiltIn {
    //--------------------------------------------------------------
    //! @brief ビルトインアセットの初期化関数
    //--------------------------------------------------------------
    void BuiltInAssets::Initialize(Tsukino::Asset::AssetManager* assetManager) {
        shaders.Initialize(assetManager);
    }
}    // namespace Tsukino::BuiltIn

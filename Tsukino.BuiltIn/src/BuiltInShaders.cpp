//--------------------------------------------------------------
//! @file   BuiltInShaders.cpp
//! @brief  ビルトインシェーダー集約クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/BuiltIn/BuiltInShaders.hpp>

#include <Tsukino/Engine/Asset/AssetManager.hpp>

#include <Tsukino/Core/Path.hpp>
// 名前空間 : Tsukino::BuiltIn
namespace Tsukino::BuiltIn {
    //--------------------------------------------------------------
    //! @brief ビルトインシェーダーの初期化関数
    //--------------------------------------------------------------
    void BuiltInShaders::Initialize(Tsukino::Asset::AssetManager* assetManager) {
        spriteVS = assetManager->Load(Tsukino::Core::Path("Tsukino.BuiltIn/Assets/Shaders/Sprite.vs.hlsl"));
        spritePS = assetManager->Load(Tsukino::Core::Path("Tsukino.BuiltIn/Assets/Shaders/Sprite.ps.hlsl"));
    }
}    // namespace Tsukino::BuiltIn

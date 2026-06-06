//--------------------------------------------------------------
//! @file   BuiltInShaders.hpp
//! @brief  ビルトインシェーダー集約クラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    class AssetManager;    // 前方宣言
}

// 名前空間 : Tsukino::BuiltIn
namespace Tsukino::BuiltIn {
    //--------------------------------------------------------------
    //! @class   BuiltInShaders
    //! @brief   ビルトインシェーダー集約クラス
    //--------------------------------------------------------------
    class BuiltInShaders {
    public:
        //--------------------------------------------------------------
        // ビルトインシェーダー集約初期化関数
        //! @param assetManager [in] アセットマネージャーへのポインタ
        //--------------------------------------------------------------
        void Initialize(Tsukino::Asset::AssetManager* assetManager);

        Tsukino::Asset::AssetHandle spriteVS;         // スプライト用頂点シェーダーのハンドル
        Tsukino::Asset::AssetHandle spritePS;         // スプライト用ピクセルシェーダーのハンドル
        Tsukino::Asset::AssetHandle modelVS;          // モデル用頂点シェーダーのハンドル
        Tsukino::Asset::AssetHandle modelPS;          // モデル用ピクセルシェーダーのハンドル
        Tsukino::Asset::AssetHandle staticModelVS;    // アニメーションなしモデル用頂点シェーダーのハンドル
        Tsukino::Asset::AssetHandle debugVS;          // デバッグ用頂点シェーダーのハンドル
        Tsukino::Asset::AssetHandle debugPS;          // デバッグ用ピクセルシェーダーのハンドル
    };
}    // namespace Tsukino::BuiltIn

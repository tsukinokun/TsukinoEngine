//--------------------------------------------------------------
//! @file   BuiltInAssets.hpp
//! @brief  ビルトインアセット集約クラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/BuiltInShaders.hpp>
#include <Tsukino/BuiltIn/BuiltInFonts.hpp>
#include <Tsukino/BuiltIn/BuiltInModels.hpp>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    class AssetManager;    // 前方宣言
}

// 名前空間 : Tsukino::BuiltIn
namespace Tsukino::BuiltIn {
    //--------------------------------------------------------------
    //! @class   BuiltInAssets
    //! @brief   ビルトインアセット集約クラス
    //--------------------------------------------------------------
    class BuiltInAssets {
    public:
        //--------------------------------------------------------------
        // ビルトインアセットの初期化関数
        //! @param assetManager [in] アセットマネージャーへのポインタ
        //--------------------------------------------------------------
        void Initialize(Tsukino::Asset::AssetManager* assetManager);

        BuiltInShaders shaders;    // ビルトインシェーダー集約クラスのインスタンス
        BuiltInFonts   fonts;      // ビルトインフォント集約クラスのインスタンス
        BuiltInModels  models;     // ビルトインモデル（立方体・平面・球）集約クラスのインスタンス
    };
}    // namespace Tsukino::BuiltIn

//--------------------------------------------------------------
//! @file   BuiltInFonts.hpp
//! @brief  ビルトインフォント集約クラスの宣言
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
    //! @class   BuiltInFonts
    //! @brief   ビルトインフォント集約クラス
    //--------------------------------------------------------------
    class BuiltInFonts {
    public:
        //--------------------------------------------------------------
        // ビルトインフォント集約初期化関数
        //! @param assetManager [in] アセットマネージャーへのポインタ
        //--------------------------------------------------------------
        void Initialize(Tsukino::Asset::AssetManager* assetManager);

        Tsukino::Asset::AssetHandle defaultFont;    // デフォルトフォントのハンドル
    };
}    // namespace Tsukino::BuiltIn

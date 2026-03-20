//--------------------------------------------------------------
//! @file   BuiltInAssets.hpp
//! @brief  ビルトインアセット集約クラスの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
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
        //! @param assets [in] アセットマネージャーへのポインタ
        //--------------------------------------------------------------
        void Initialize(Tsukino::Asset::AssetManager* assets);
    };
}    // namespace Tsukino::BuiltIn

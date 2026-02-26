//--------------------------------------------------------------
//! @file	AssetType.hpp
//! @brief  アセットの種類を定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @enum   AssetType
    //! @brief  アセットの種類を定義する列挙型
    //--------------------------------------------------------------
    enum class AssetType : u8 {
        None = 0,    // 無効なアセット
        Texture,     // テクスチャ
        Shader,      // シェーダー
        Mesh,        // メッシュ
        Material,    // マテリアル
        Audio,       // オーディオ
    };

}    // namespace Tsukino::Asset

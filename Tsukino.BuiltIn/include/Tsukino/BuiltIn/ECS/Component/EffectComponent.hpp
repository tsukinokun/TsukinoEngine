#pragma once
//--------------------------------------------------------------
//! @file   EffectComponent.hpp
//! @brief  エフェクトコンポーネントの定義
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Core/Path.hpp>
#include <vector>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @struct  TextureBinding
    //! @brief   エフェクトテクスチャのバインド情報
    //--------------------------------------------------------------
    struct TextureBinding {
        Tsukino::Asset::AssetHandle original;    //!< 元のテクスチャアセットハンドル
        Tsukino::Asset::AssetHandle replacement; //!< 差し替え先のテクスチャアセットハンドル
        Tsukino::Core::Path texturePath;         //!< テクスチャパス
        int layer;                               //!< テクスチャレイヤー（0-31）
        bool isOverride = false;                 //!< 差し替えモードかどうか
    };

    //--------------------------------------------------------------
    //! @struct  EffectComponent
    //! @brief  Effekseerエフェクトを再生するためのコンポーネント
    //--------------------------------------------------------------
    struct EffectComponent {
        Tsukino::Asset::AssetHandle effectAsset;          //!< .efk アセット
        int                         handle    = -1;       //!< Effekseer インスタンスハンドル
        float                       playSpeed = 1.0f;     //!< 再生速度
        bool                        looping   = false;    //!< ループ再生フラグ
        bool                        stopped   = false;    //!< 停止フラグ
        bool                        active    = false;    //!< 再生中フラグ
        std::vector<TextureBinding> textureBindings;      //!< テクスチャバインド情報
    };
}    // namespace Tsukino::BuiltIn::ECS
#pragma once
//--------------------------------------------------------------
//! @file   EffectComponent.hpp
//! @brief  エフェクトコンポーネントの定義
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

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
    };
}    // namespace Tsukino::BuiltIn::ECS

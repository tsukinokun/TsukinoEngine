#pragma once
//--------------------------------------------------------------
//! @file   AudioComponent.hpp
//! @brief  オーディオコンポーネントの定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @struct AudioComponent
    //! @brief  音声を再生するためのコンポーネント
    //--------------------------------------------------------------
    struct AudioComponent {
        Tsukino::Asset::AssetHandle audioHandle = Tsukino::Asset::AssetHandle::Invalid(); //!< 再生するオーディオアセットのハンドル
        bool playOnAwake = false; //!< 生成時に自動再生するかどうか
        bool loop = false;        //!< ループ再生するかどうか
        float volume = 1.0f;      //!< 音量 (0.0f ~ 1.0f)
        float pitch = 0.0f;       //!< ピッチ (再生速度・音程)
        float pan = 0.0f;         //!< パン (左右の定位)

        bool isPlaying = false;   //!< 現在再生中かどうか
        bool playTrigger = false; //!< 再生トリガー (trueにすると再生される)
        bool stopTrigger = false; //!< 停止トリガー (trueにすると停止される)
    };
}

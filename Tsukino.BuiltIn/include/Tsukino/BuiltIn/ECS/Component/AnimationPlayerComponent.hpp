//-------------------------------------------------------------
//! @file   AnimationPlayerComponent.hpp
//! @brief  AnimationPlayerComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  AnimationPlayerComponent
    //! @brief  アニメーションの再生状態を管理するクラス
    //-------------------------------------------------------------
    struct AnimationPlayerComponent {
        Tsukino::Asset::AssetHandle current_clip_id;    // リソースマネージャ内のID
        float                       elapsed_time;       // 現在の再生時間（秒）
        float                       playback_speed;     // 1.0f = 等速
        bool                        is_looping;         // ループ再生フラグ
        bool                        is_playing;         // 一時停止フラグ
    };
}    // namespace Tsukino::BuiltIn::ECS

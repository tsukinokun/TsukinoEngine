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
        u32                         animation_index = 0;// 再生するアニメーションのインデックス
        float                       elapsed_time;       // 現在の再生時間（秒）
        float                       playback_speed;     // 1.0f = 等速
        bool                        is_looping;         // ループ再生フラグ
        bool                        is_playing;         // 一時停止フラグ

        // ループしない（is_looping=false）クリップが最後まで再生し終えたか。
        // AnimationSystemが毎フレーム再計算するポーリング値であり、終了した瞬間だけtrueになる
        // イベントではない（クリップが切り替わるまでtrueが継続する）
        bool                        is_finished = false;
    };
}    // namespace Tsukino::BuiltIn::ECS

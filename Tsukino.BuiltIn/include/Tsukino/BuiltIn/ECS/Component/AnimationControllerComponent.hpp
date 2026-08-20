//-------------------------------------------------------------
//! @file   AnimationControllerComponent.hpp
//! @brief  AnimationControllerComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct  AnimationControllerComponent
    //! @brief  アニメーションの遷移状態を管理するクラス
    //-------------------------------------------------------------
    struct AnimationControllerComponent {
        float blend_alpha      = 0.0f;     // 遷移の進捗 (0~1)
        bool  is_transitioning = false;    // 現在ブレンド中か

        //-------------------------------------------------------------
        //! @struct  NextAnimation
        //! @brief  次のアニメーションの情報
        //-------------------------------------------------------------
        struct NextAnimation {
            Tsukino::Asset::AssetHandle clip;                       // リソースマネージャ内のID
            u32                         animation_index = 0;        // 次に再生するアニメーションのインデックス
            float                       fade_time       = 0.0f;     // フェード時間（秒）
            bool                        immediate       = false;    // 即座に切り替えるか、今のが終わってからか
            bool                        is_looping      = true;     // 切り替え後のループ再生フラグ
            float                       clip_start_time = 0.0f;     // クリップ内の再生レンジ開始（秒）。既定0で全体再生
            float                       clip_end_time   = 0.0f;     // クリップ内の再生レンジ終了（秒）。0以下ならクリップ末尾まで
            bool                        in_place        = false;    // trueならルートモーションノードの水平移動を無視する
        } next;

        //-------------------------------------------------------------
        //! @struct  OutgoingAnimation
        //! @brief  ブレンド元（フェードアウト中）のアニメーションのスナップショット
        //-------------------------------------------------------------
        struct OutgoingAnimation {
            Tsukino::Asset::AssetHandle clip;                       // リソースマネージャ内のID
            u32                         animation_index = 0;        // 再生するアニメーションのインデックス
            float                       elapsed_time    = 0.0f;     // 遷移開始時点から独立して進む経過時間（秒）
            bool                        is_looping      = true;     // ループ再生フラグ
            float                       clip_start_time = 0.0f;     // クリップ内の再生レンジ開始（秒）。既定0で全体再生
            float                       clip_end_time   = 0.0f;     // クリップ内の再生レンジ終了（秒）。0以下ならクリップ末尾まで
            bool                        in_place        = false;    // trueならルートモーションノードの水平移動を無視する
        } outgoing;
    };

}    // namespace Tsukino::BuiltIn::ECS

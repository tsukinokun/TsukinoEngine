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
        float blend_alpha;         // 遷移の進捗 (0~1)
        bool  is_transitioning;    // 現在ブレンド中か

        //-------------------------------------------------------------
        //! @struct  NextAnimation
        //! @brief  次のアニメーションの情報
        //-------------------------------------------------------------
        struct NextAnimation {
            Tsukino::Asset::AssetHandle clip;             // リソースマネージャ内のID
            u32                         animation_index = 0; // 次に再生するアニメーションのインデックス
            float                       fade_time;        // フェード時間（秒）
            bool                        immediate;        // 即座に切り替えるか、今のが終わってからか
        } next;
    };

}    // namespace Tsukino::BuiltIn::ECS

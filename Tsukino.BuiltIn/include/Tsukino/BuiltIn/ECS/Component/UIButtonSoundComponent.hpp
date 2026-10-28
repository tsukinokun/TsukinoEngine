//----------------------------------------------------------------------------
//! @file   UIButtonSoundComponent.hpp
//! @brief  画面UIのボタンの音（重なったとき・クリックしたとき）のコンポーネント
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Engine/Asset/AssetRef.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //! 画面UIのボタンの音です。
    //! @note   PointerTargetComponent と一緒に付ける。UIButtonSoundSystem が hoverEntered・clicked を読んで鳴らす。
    //!         UIButtonFeedbackComponent も付いていて interactable が false の間は鳴らさない。
    //!         音を設定しなければ（無効なハンドルなら）その音は鳴らさない
    struct UIButtonSoundComponent {
        Tsukino::Asset::AssetRef hoverSound;             // カーソルが重なり始めたときの音。Prefab JSON ではパス文字列で書ける
        Tsukino::Asset::AssetRef clickSound;             // クリックしたときの音
        float                    hoverVolume = 1.0f;    // 重なったときの音量（0 〜 1）
        float                    clickVolume = 1.0f;    // クリックしたときの音量（0 〜 1）
        u8                       bus         = 0;       // 鳴らすバス（AudioManager::SetBusVolume で効果音の音量をまとめて変える）
    };
}    // namespace Tsukino::BuiltIn::ECS

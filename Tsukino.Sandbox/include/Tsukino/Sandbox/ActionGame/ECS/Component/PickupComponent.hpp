//-------------------------------------------------------------
//! @file   PickupComponent.hpp
//! @brief  PickupComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <string>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    //-------------------------------------------------------------
    //! @struct PickupComponent
    //! @brief  ワールドに落ちていて拾えるエンティティに付与するコンポーネント。
    //!         PickupSystemがプレイヤーとの距離判定・ハイライト演出・UI表示・
    //!         Fキー取得処理をまとめて行う
    //-------------------------------------------------------------
    struct PickupComponent {
        float        radius      = 150.0f;      //!< プレイヤーがこの距離まで近づくと拾える（1ユニット≒1cm規約）
        std::wstring displayName = L"アイテム";    //!< UIに出す名前
        float        labelHeight = 120.0f;      //!< ラベルを出すアイテム原点からの高さ

        //-------------------------------------------------------------
        // 演出用。PickupSystemが対象になっている間だけ進め、外れたら巻き戻す
        //-------------------------------------------------------------
        float highlightBlend = 0.0f;    //!< 0=消灯, 1=完全点灯。0↔1を滑らかに行き来する
        float pulseTime      = 0.0f;    //!< 白発光の脈動用の経過時間
    };
}    // namespace ActionGame::ECS

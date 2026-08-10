//-------------------------------------------------------------
//! @file   RoundComponent.hpp
//! @brief  RoundComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <array>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @enum class Hand
    //! @brief  チンチロの役。値が大きいほど強い役として比較に使う
    //-------------------------------------------------------------
    enum class Hand : u8 {
        None,       //!< 未判定
        MeNashi,    //!< 目なし（バラバラ・役なし）
        HiFuMi,     //!< 1,2,3（役なし）
        Me,         //!< 通常の目（2つ揃い＋1つ）
        Shigoro,    //!< 4,5,6
        Arashi,     //!< ゾロ目（2〜6）
        PinZoro     //!< ゾロ目（1,1,1・最強）
    };

    //-------------------------------------------------------------
    //! @struct RoundComponent
    //! @brief  1プレイヤー分、3つのサイコロをまとめて役判定するためのデータ
    //-------------------------------------------------------------
    struct RoundComponent {
        std::array<Tsukino::ECS::Entity, 3> dice{};    //!< この手番を構成する3つのDiceエンティティ

        Hand kind     = Hand::None;    //!< 判定された役
        u8   subValue = 0;             //!< Meの場合の目、Arashi/PinZoroのゾロ目の値
        bool judged   = false;         //!< 役の判定が完了しているか
    };

}    // namespace LuckGameSampleScene::ECS

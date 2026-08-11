//-------------------------------------------------------------
//! @file   DiceComponent.hpp
//! @brief  DiceComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>
#include <hlsl++.h>

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {

    //-------------------------------------------------------------
    //! @enum class DiceRollState
    //! @brief  サイコロの転がり状態
    //-------------------------------------------------------------
    enum class DiceRollState {
        Idle,       //!< 静止・投げられるのを待っている状態
        Rolling,    //!< 投げられて転がっている最中
        Settled     //!< 静止判定が確定した（出目を読み取ってよい状態）
    };

    //-------------------------------------------------------------
    //! @struct DiceComponent
    //! @brief  サイコロ1個分の出目・転がり状態データ
    //-------------------------------------------------------------
    struct DiceComponent {
        // 各面のローカル法線と、対応する出目（対面の和が7になる一般的なサイコロ配置）
        hlslpp::float3 faceNormal[6] = {
            hlslpp::float3( 1.0f,  0.0f,  0.0f),
            hlslpp::float3(-1.0f,  0.0f,  0.0f),
            hlslpp::float3( 0.0f,  1.0f,  0.0f),
            hlslpp::float3( 0.0f, -1.0f,  0.0f),
            hlslpp::float3( 0.0f,  0.0f,  1.0f),
            hlslpp::float3( 0.0f,  0.0f, -1.0f),
        };
        u8 faceValue[6] = {1, 6, 2, 5, 3, 4};

        DiceRollState state         = DiceRollState::Idle;    //!< 現在の転がり状態
        float         settleTimer   = 0.0f;                   //!< 静止継続時間の積算（秒）
        u8            confirmedValue = 0;                     //!< 確定した出目（1〜6）
        bool          confirmed      = false;                 //!< 出目が確定済みかどうか
    };

}    // namespace LuckGameSampleScene::ECS

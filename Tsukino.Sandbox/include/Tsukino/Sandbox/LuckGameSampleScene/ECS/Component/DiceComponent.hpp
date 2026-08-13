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
        Idle,         //!< 静止・投げられるのを待っている状態（未使用に近いが初期値として残す）
        Respawning,   //!< お椀中心上空へテレポート（Kinematic化）中。1〜2フレームでHoveringに遷移する
        Hovering,     //!< 空中で静止＋回転しながら、投下（スペース入力）を待っている状態
        Rolling,      //!< 投げられて（落とされて）転がっている最中
        Settled       //!< 静止判定が確定した（出目を読み取ってよい状態）
    };

    //-------------------------------------------------------------
    //! @struct DiceComponent
    //! @brief  サイコロ1個分の出目・転がり状態データ
    //-------------------------------------------------------------
    struct DiceComponent {
        // 各面のローカル法線と、対応する出目。
        // Dice.fbxモデルの各ローカル軸を実際にワールド上向きへ強制回転させ、
        // トップダウンカメラ越しに実測した値（対面の和は7になる。同一条件で2回再現確認済み）。
        // 以前は別の実測値({2,5,4,3,6,1})が入っていたが、この環境固有の
        // スクリーンショット取り違え問題（8章参照）の影響を受けていた可能性が高く、
        // 再検証の結果に合わせて修正済み。
        hlslpp::float3 faceNormal[6] = {
            hlslpp::float3( 1.0f,  0.0f,  0.0f),
            hlslpp::float3(-1.0f,  0.0f,  0.0f),
            hlslpp::float3( 0.0f,  1.0f,  0.0f),
            hlslpp::float3( 0.0f, -1.0f,  0.0f),
            hlslpp::float3( 0.0f,  0.0f,  1.0f),
            hlslpp::float3( 0.0f,  0.0f, -1.0f),
        }; 
       u8 faceValue[6] = {5, 2, 1, 6, 3, 4};

        DiceRollState state         = DiceRollState::Idle;    //!< 現在の転がり状態
        float         settleTimer   = 0.0f;                   //!< 静止継続時間の積算（秒）
        u8            confirmedValue = 0;                     //!< 確定した出目（1〜6）
        bool          confirmed      = false;                 //!< 出目が確定済みかどうか
    };

}    // namespace LuckGameSampleScene::ECS

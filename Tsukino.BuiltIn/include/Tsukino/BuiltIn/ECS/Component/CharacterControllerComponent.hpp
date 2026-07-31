//-------------------------------------------------------------
//! @file   CharacterControllerComponent.hpp
//! @brief  CharacterControllerComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //-------------------------------------------------------------
    //! @struct CharacterControllerComponent
    //! @brief  CharacterVirtualによるキャラクター移動制御を表すコンポーネント
    //-------------------------------------------------------------
    struct CharacterControllerComponent {
        // 形状パラメータ（生成時のみ使用）
        float radius      = 0.3f;     //!< カプセル半径
        float halfHeight  = 0.6f;     //!< カプセル円柱部分の半分の高さ
        float maxSlopeDeg = 45.0f;    //!< 登れる最大斜面角度
        float mass        = 70.0f;    //!< 押し出し計算用の仮想質量

        // Jolt内部でCharacterVirtualを生成するためのトリガー
        bool isInitialized = false;

        // 毎フレーム入力（他システムが毎フレーム上書きする想定）
        hlslpp::float3 moveInput     = {0, 0, 0};    //!< 水平方向の希望移動速度（m/s）
        bool           jumpRequested = false;        //!< ジャンプ要求フラグ（消費後PhysicsSystemがfalseに戻す）
        float          jumpSpeed     = 6.0f;         //!< ジャンプ初速

        // 内部状態（PhysicsSystemが毎フレーム更新。読み取り専用として扱うこと）
        hlslpp::float3 verticalVelocity = {0, 0, 0};    //!< 重力・ジャンプによる縦方向速度
        bool           isGrounded       = false;        //!< 接地しているか
    };
}    // namespace Tsukino::BuiltIn::ECS

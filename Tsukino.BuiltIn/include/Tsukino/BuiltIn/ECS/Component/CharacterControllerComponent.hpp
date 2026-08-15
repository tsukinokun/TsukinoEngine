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
        float radius        = 0.3f;     //!< カプセル半径
        float halfHeight    = 0.6f;     //!< カプセル円柱部分の半分の高さ
        float maxSlopeDeg   = 45.0f;    //!< 登れる最大斜面角度
        float mass          = 70.0f;    //!< 押し出し計算用の仮想質量
        float gravityFactor = 1.0f;     //!< 重力の掛かり具合（RigidbodyComponent::gravityFactorと同様。ワールドの単位スケールに応じて調整する）

        //-------------------------------------------------------------
        // カプセル中心の、Transform位置からのローカルオフセット（Unityの
        // CharacterController.centerと同様の役割）。
        // (0,0,0)のままなら従来通りTransform位置＝カプセル中心。
        // (0, halfHeight+radius, 0) を指定すると、Transform位置がカプセル底面
        // （＝足元/接地位置）を表すようになり、足元原点のモデルをそのまま
        // 同じTransformで描画しても位置が一致しやすくなる。
        //-------------------------------------------------------------
        hlslpp::float3 centerOffset = {0.0f, 0.0f, 0.0f};

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

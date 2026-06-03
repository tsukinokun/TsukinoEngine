//-------------------------------------------------------------
//! @file   RigidbodyComponent.hpp
//! @brief  RigidbodyComponentクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <hlsl++.h>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //-------------------------------------------------------------
    //! @enum class RigidbodyType
    //! @brief  剛体の種類を定義する列挙型
    //-------------------------------------------------------------
    enum class RigidbodyType {
        Static,       // 動かない（床、壁）
        Kinematic,    // プログラムから座標を直接制御する（動く床、アニメーションする扉）
        Dynamic       // 物理シミュレーションに従う（落下、衝突反発）
    };

    //-------------------------------------------------------------
    //! @struct RigidbodyComponent
    //-------------------------------------------------------------
    struct RigidbodyComponent {
        RigidbodyType type = RigidbodyType::Dynamic;

        float mass          = 1.0f;    // 質量
        float friction      = 0.5f;    // 摩擦係数
        float restitution   = 0.0f;    // 反発係数
        float gravityFactor = 1.0f;    // デフォルトは標準重力

        hlslpp::float3 linearVelocity  = {0, 0, 0};    // 線速度
        hlslpp::float3 angularVelocity = {0, 0, 0};    // 角速度

        // Jolt内部でボディを生成するためのトリガー
        bool isInitialized = false;

        // 着地判定
        bool  isGrounded          = false;    //!< 地面に接触しているか（PhysicsSystemが毎フレーム更新）
        float groundCheckDistance = 0.1f;     //!< 足元チェックのオフセット（コライダー底面からの距離）
        float groundCheckRadius   = 0.2f;     //!< 足元チェックBoxの水平サイズ
    };
}    // namespace Tsukino::BuiltIn::ECS

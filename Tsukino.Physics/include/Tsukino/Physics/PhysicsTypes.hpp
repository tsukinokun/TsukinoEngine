//----------------------------------------------------------------------------
//! @file   PhysicsTypes.hpp
//! @brief  PhysicsWorld が受け渡しに使うデータ型の定義
//! @detail 物理エンジン（Jolt）の型を一切含みません。座標や回転は hlslpp、
//!         エンティティの同一性は uint64_t のユーザーデータで表現します。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Physics/BodyHandle.hpp>

#include <hlsl++.h>

#include <cstdint>

// 名前空間 : Tsukino::Physics
namespace Tsukino::Physics {

    //------------------------------------------------------------------------
    //! 衝突判定に使用する形状の種類
    //------------------------------------------------------------------------
    enum class ShapeType : uint8_t {
        Box,           //!< 直方体
        Sphere,        //!< 球
        Capsule,       //!< カプセル
        Heightfield    //!< ハイトフィールド（地形）
    };

    //------------------------------------------------------------------------
    //! ボディの運動タイプ
    //------------------------------------------------------------------------
    enum class MotionType : uint8_t {
        Static,       //!< 動かない。他から押されもしない
        Kinematic,    //!< 位置を外部から与える。物理演算では動かない
        Dynamic       //!< 物理演算で動く
    };

    //------------------------------------------------------------------------
    //! 移動・回転の許可軸を表すビットマスク
    //------------------------------------------------------------------------
    enum class DofMask : uint8_t {
        None         = 0,         //!< 全軸を凍結
        TranslationX = 1 << 0,    //!< X 方向の移動を許可
        TranslationY = 1 << 1,    //!< Y 方向の移動を許可
        TranslationZ = 1 << 2,    //!< Z 方向の移動を許可
        RotationX    = 1 << 3,    //!< X 軸まわりの回転を許可
        RotationY    = 1 << 4,    //!< Y 軸まわりの回転を許可
        RotationZ    = 1 << 5,    //!< Z 軸まわりの回転を許可
        All          = 0x3f       //!< 全軸を許可
    };

    //! 2つのマスクの論理和を返します。
    //! @param  [in] lhs 左辺
    //! @param  [in] rhs 右辺
    //! @return 論理和
    inline constexpr DofMask operator|(DofMask lhs, DofMask rhs) {
        return static_cast<DofMask>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
    }

    //! 2つのマスクの論理積を返します。
    //! @param  [in] lhs 左辺
    //! @param  [in] rhs 右辺
    //! @return 論理積
    inline constexpr DofMask operator&(DofMask lhs, DofMask rhs) {
        return static_cast<DofMask>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
    }

    //! マスクのビットを反転して返します。
    //! @param  [in] mask 反転対象
    //! @return 反転結果（All の範囲へ丸める）
    inline constexpr DofMask operator~(DofMask mask) {
        return static_cast<DofMask>(~static_cast<uint8_t>(mask) & static_cast<uint8_t>(DofMask::All));
    }

    //! 論理積を代入します。
    //! @param  [in,out] lhs 代入先
    //! @param  [in]     rhs 右辺
    //! @return 代入後の lhs への参照
    inline constexpr DofMask& operator&=(DofMask& lhs, DofMask rhs) {
        lhs = lhs & rhs;
        return lhs;
    }

    //------------------------------------------------------------------------
    //! 形状の指定
    //------------------------------------------------------------------------
    struct ShapeDesc {
        ShapeType type = ShapeType::Box;    //!< 形状の種類

        //--------------------------------------------------------------------
        // 各形状のサイズデータ
        // Box    : 各軸の半分サイズ(Half Extents)
        // Sphere : x = 半径
        // Capsule: x = 半径, y = 半分の高さ
        //--------------------------------------------------------------------
        hlslpp::float3 extent = {0.5f, 0.5f, 0.5f};

        //--------------------------------------------------------------------
        // ハイトフィールド専用データ（type == ShapeType::Heightfield のみ使用）
        //--------------------------------------------------------------------
        const float*   heightSamples = nullptr;               //!< 高さサンプル配列（行優先）
        uint32_t       heightSize    = 0;                     //!< 1辺あたりのサンプル数
        hlslpp::float3 heightOffset  = {0.0f, 0.0f, 0.0f};    //!< シェイプローカルでのオフセット
        hlslpp::float3 heightScale   = {1.0f, 1.0f, 1.0f};    //!< x,z: サンプル間隔 / y: 高さの倍率
    };

    //------------------------------------------------------------------------
    //! ボディ生成時の指定
    //------------------------------------------------------------------------
    struct BodyDesc {
        ShapeDesc          shape;                                       //!< 形状
        hlslpp::float3     position = {0.0f, 0.0f, 0.0f};                //!< 生成位置（ワールド）
        hlslpp::quaternion rotation = hlslpp::quaternion::identity();    //!< 生成時の向き
        MotionType         motion   = MotionType::Static;                //!< 運動タイプ
        bool               isSensor = false;                             //!< true なら反発せず判定のみ行う

        float   mass          = 1.0f;            //!< 質量（Dynamic のみ意味を持つ）
        float   friction      = 0.2f;            //!< 摩擦係数
        float   restitution   = 0.0f;            //!< 反発係数
        float   gravityFactor = 1.0f;            //!< 重力の倍率
        DofMask allowedDofs   = DofMask::All;    //!< 許可する移動・回転軸

        //--------------------------------------------------------------------
        // mass と allowedDofs を明示的に適用するかどうか
        // false の場合は物理エンジンの既定（形状の密度から算出した質量・全軸許可）に任せる
        //--------------------------------------------------------------------
        bool overrideMassProperties = false;

        uint64_t userData = 0;    //!< 呼び出し側がボディに紐付ける任意の値
    };

    //------------------------------------------------------------------------
    //! ボディの現在値をまとめて取り出したもの
    //------------------------------------------------------------------------
    struct BodyState {
        hlslpp::float3     position        = {0.0f, 0.0f, 0.0f};                //!< 位置
        hlslpp::quaternion rotation        = hlslpp::quaternion::identity();    //!< 向き
        hlslpp::float3     linearVelocity  = {0.0f, 0.0f, 0.0f};                //!< 並進速度
        hlslpp::float3     angularVelocity = {0.0f, 0.0f, 0.0f};                //!< 角速度
    };

    //------------------------------------------------------------------------
    //! キャラクターコントローラー生成時の指定
    //------------------------------------------------------------------------
    struct CharacterDesc {
        float radius      = 0.5f;     //!< カプセル半径
        float halfHeight  = 0.5f;     //!< カプセル円柱部分の半分の高さ
        float maxSlopeDeg = 45.0f;    //!< 登坂可能な最大傾斜角（度）
        float mass        = 70.0f;    //!< 質量

        //--------------------------------------------------------------------
        // カプセル中心を Transform 位置からずらす量
        // （Unity の CharacterController.center と同様。例えば (0, halfHeight+radius, 0)
        //   を指定すると Transform 位置＝カプセル底面＝足元、を表せる）
        //--------------------------------------------------------------------
        hlslpp::float3 centerOffset = {0.0f, 0.0f, 0.0f};

        hlslpp::float3     position = {0.0f, 0.0f, 0.0f};                //!< 生成位置（ワールド）
        hlslpp::quaternion rotation = hlslpp::quaternion::identity();    //!< 生成時の向き
        uint64_t           userData = 0;                                 //!< 呼び出し側が紐付ける任意の値
    };

    //------------------------------------------------------------------------
    //! キャラクターの1ステップ分の入力
    //------------------------------------------------------------------------
    struct CharacterInput {
        hlslpp::float3     linearVelocity = {0.0f, 0.0f, 0.0f};                //!< 水平＋垂直を合成した速度
        hlslpp::quaternion rotation       = hlslpp::quaternion::identity();    //!< 反映したい向き
    };

    //------------------------------------------------------------------------
    //! キャラクターの1ステップ分の結果
    //------------------------------------------------------------------------
    struct CharacterOutput {
        hlslpp::float3     position         = {0.0f, 0.0f, 0.0f};                //!< 更新後の位置
        hlslpp::quaternion rotation         = hlslpp::quaternion::identity();    //!< 更新後の向き
        float              verticalVelocity = 0.0f;                              //!< 更新後の実際の縦速度
        bool               isGrounded       = false;                             //!< 接地しているか
    };

    //------------------------------------------------------------------------
    //! 1件分の接触情報
    //------------------------------------------------------------------------
    struct ContactRecord {
        uint64_t       userDataA = 0;                     //!< ボディ1 側のユーザーデータ
        uint64_t       userDataB = 0;                     //!< ボディ2 側のユーザーデータ
        hlslpp::float3 normal    = {0.0f, 0.0f, 0.0f};    //!< ボディ1 から見た接触法線
    };

}    // namespace Tsukino::Physics

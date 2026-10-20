//--------------------------------------------------------------
//! @file   BoneFollowColliderComponent.hpp
//! @brief  当たり判定をボーンへ付いていかせるコンポーネント
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <hlsl++.h>

#include <cstdint>
#include <string>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @struct BoneFollowColliderComponent
    //! @brief  CollisionComponentのoffsetPositionを、指定したボーンが動いた分だけ毎フレームずらす
    //! @note   アニメーションで胴体が根元（Transform）から大きく離れるモデル（倒れて転がる等）で、
    //!         見えている体と当たり判定を合わせるためのもの。BoneFollowColliderSystemが書き込む。
    //!         ずらす量は「最初に取れたフレームのボーン位置」からの差。最初のフレームのoffsetPositionを
    //!         基準として覚えるので、アプリ側が生成直後にoffsetPositionを書き換えても（大きさの倍率等）そのまま効く。
    //!         Kinematic/Static向け。DynamicはPhysicsSystemがTransformを物理側から上書きするので使わない
    //--------------------------------------------------------------
    struct BoneFollowColliderComponent {
        std::string boneName;                 //!< 付いていくボーンの名前（ModelComponentのモデルのノード名）
        bool        followHeight = false;     //!< 高さも追うか。falseなら水平だけ（倒れた体の持ち上がりで判定が浮かないように）
        float       followSpeed  = 12.0f;     //!< ボーンへ追いつく速さ（1/秒、指数平滑）。0以下なら毎フレームぴったり合わせる

        // --- 実行時の状態（保存しない） ---
        hlslpp::float3              baseOffset{0.0f, 0.0f, 0.0f};          //!< 最初のフレームのoffsetPosition
        hlslpp::float3              restBonePosition{0.0f, 0.0f, 0.0f};    //!< 最初に取れたフレームのボーン位置（モデル空間）
        hlslpp::float3              currentShift{0.0f, 0.0f, 0.0f};        //!< 今ずらしている量
        bool                        hasRest = false;                       //!< baseOffset・restBonePositionを取ったか
        Tsukino::u32                boneNodeIndex = UINT32_MAX;            //!< boneNameを解決したnodeIndex
        Tsukino::Asset::AssetHandle resolvedAgainstModel;                  //!< boneNodeIndexを解決したときのモデル
    };
}    // namespace Tsukino::BuiltIn::ECS

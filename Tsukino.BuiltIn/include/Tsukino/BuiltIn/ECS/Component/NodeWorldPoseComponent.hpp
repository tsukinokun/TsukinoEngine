//-------------------------------------------------------------
//! @file   NodeWorldPoseComponent.hpp
//! @brief  NodeWorldPoseComponent構造体の宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Physics/SpringBone/SpringBoneData.hpp>

#include <vector>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @struct NodeWorldPoseComponent
    //! @brief  AnimationSystemが毎フレーム計算する、各ノードのワールド姿勢（位置・回転のみ）を
    //!         他エンティティ（武器のボーンアタッチ等）から参照できるよう公開するコンポーネント。
    //!         modelData.nodesのインデックスに対応し、モデルローカル空間（所有エンティティ自身の
    //!         TransformComponentは未反映）であることに注意。
    //-------------------------------------------------------------
    struct NodeWorldPoseComponent {
        std::vector<Tsukino::Physics::WorldPose> poses;
    };

}    // namespace Tsukino::BuiltIn::ECS

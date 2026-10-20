//--------------------------------------------------------------
//! @file   BoneFollowColliderSystem.hpp
//! @brief  BoneFollowColliderSystemクラスの宣言
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @class  BoneFollowColliderSystem
    //! @brief  BoneFollowColliderComponentを持つエンティティの当たり判定（CollisionComponent::offsetPosition）を、
    //!         指定ボーンが最初のフレームの位置から動いた分だけずらすシステム。
    //!         Kinematic/StaticのボディはPhysicsSystemが毎フレーム Transform＋offsetPosition へ動かすので、
    //!         offsetPositionを書き換えれば判定が付いてくる
    //! @note   AnimationSystemがその回のボーン行列（NodeWorldMatrixComponent）を書いた後、
    //!         PhysicsSystemの同期より前に実行すること。判定の位置を読む処理（攻撃の当たり等）があるなら、それより前
    //--------------------------------------------------------------
    class BoneFollowColliderSystem : public Tsukino::ECS::ISystem {
    public:
        //--------------------------------------------------------------
        //! @brief 更新処理
        //--------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}    // namespace Tsukino::BuiltIn::ECS

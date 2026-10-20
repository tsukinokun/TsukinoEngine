//--------------------------------------------------------------
//! @file   BoneFollowColliderComponentSerialization.hpp
//! @brief  BoneFollowColliderComponentのcerealシリアライズ定義
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/BoneFollowColliderComponent.hpp>

#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  BoneFollowColliderComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const BoneFollowColliderComponent& follow) {
        // 基準位置・解決したnodeIndexなどの実行時の状態は保存しない
        archive(cereal::make_nvp("boneName", follow.boneName),
                cereal::make_nvp("followHeight", follow.followHeight),
                cereal::make_nvp("followSpeed", follow.followSpeed));
    }

    //--------------------------------------------------------------
    //! @brief  BoneFollowColliderComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, BoneFollowColliderComponent& follow) {
        archive(follow.boneName, follow.followHeight, follow.followSpeed);
        // 読み直したら基準を取り直す
        follow.hasRest              = false;
        follow.boneNodeIndex        = UINT32_MAX;
        follow.resolvedAgainstModel = Tsukino::Asset::AssetHandle::Invalid();
    }

}    // namespace Tsukino::BuiltIn::ECS
